#include "stdwx.h"
#include <wx/stopwatch.h>
#include "cExecutor.h"


cExecutor::cExecutor() : m_CommPort(), m_Timer(this) {
	this->Bind(wxEVT_TIMER, &cExecutor::OnTimer, this, m_Timer.GetId());
	m_Timer.Start(100);	// millisecond interval
}

cExecutor::~cExecutor() {
	m_Running = false;
	m_Timer.Stop();
};
#if defined(USE_FSA_EXEC)
wxString cExecutor::StateName(eCmdState s) {
	switch (s) {
		case STATE_IDLE:    return "STATE_IDLE";
		case STATE_START:   return "STATE_START";
		case STATE_WRITE:   return "STATE_WRITE";
		case STATE_READ:    return "STATE_READ";
		case STATE_SUCCESS: return "STATE_SUCCESS";
		case STATE_ERROR:   return "STATE_ERROR";
	}
	return "???";
}

bool cExecutor::StartSendCommand(const sCommand& vStep, size_t length, long TimeoutMs) {
	if (m_CmdState != STATE_IDLE) {
//		LogMe("Warning: A command is already executing.\n", true);
		return false;
	}

	m_CurrentCmd	= vStep;
	m_CmdLength		= length;
	m_TimeoutMs		= (TimeoutMs <= 0) ? 500 : TimeoutMs;
	m_RetryCount	= 0;
	m_ResponseBuffer.clear();

	m_CmdState = STATE_START;
	return true;
}
bool cExecutor::UpdateSendCommand() {
	// Se non è in esecuzione o se è stato richiesto lo stop dall'esterno
	if (m_CmdState == STATE_IDLE) return false;

	if (!m_Running) {
		m_CmdState = STATE_IDLE;
		return false;
	}

	switch (m_CmdState) {
		case STATE_START:
			m_swTotCmd.Start(0);
			m_CmdState = STATE_WRITE;
//			break;

		case STATE_WRITE:
			// Tentativo di scrittura sulla porta
			if (!m_CommPort.Write(&m_CurrentCmd, m_CmdLength, 1000)) {
				LogMe("Hardware error or timeout while writing.\n", true);
				m_Running = false;
				m_CmdState = STATE_ERROR;
				break;
			}
			//LogMe(wxString::Format("Attempt %d,", ++m_RetryCount), false);
			m_CmdState = STATE_READ;
//			break;

		case STATE_READ:
			{
				size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, m_CurrentCmd.m_Cmd, m_TimeoutMs);	// Lettura non bloccante (o vincolata al TimeoutMs)

				if (bytesRead >= sizeof(AnswerHeader)) {
					const AnswerHeader* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
					if (ptrHeader) {
						if (m_ptrAnswerShow) m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);

						bool isOk = (ptrHeader->m_Result == eCmdOk);
						if (m_CurrentCmd.m_Cmd != ptrHeader->m_Cmd) {
							LogMe(wxString::Format("\tAAA: Answer non coherent %d != %d\n", (int)m_CurrentCmd.m_Cmd, (int)ptrHeader->m_Cmd), false);
							isOk = false;
						}

						//LogMe(wxString::Format("Answer is %s ...", isOk ? "True\n" : "False"), false);
						m_CmdState = isOk ? STATE_SUCCESS : STATE_WRITE;
					}
				} else {
					// Timeout scaduto
					LogMe(wxString::Format("Timeout expired (%ld ms). Retransmitting...\n", m_TimeoutMs), true);
					if (m_RetryCount > 50) {
						LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
						m_Running = false;
						m_CmdState = STATE_ERROR;
					} else {
						m_CmdState = STATE_WRITE;	// Torna a scrivere per ritrasmettere
					}
				}
			}
			break;

		case STATE_SUCCESS:
			LogMe(wxString::Format("  Completed in %ld ms.\n", m_swTotCmd.Time()), false);
			m_CmdState = STATE_IDLE;
			break;

		case STATE_ERROR:
			LogMe(wxString::Format("  Failed/Aborted after %ld ms.\n", m_swTotCmd.Time()), true);
			m_CmdState = STATE_IDLE;
			break;

		default:
			m_CmdState = STATE_IDLE;
			break;
	}

	return (m_CmdState != STATE_IDLE);
}
#else
void cExecutor::SendCommand(const sCommand& vStep, size_t length, long TimeoutMs) {
	bool		Success		= false;
	int			retryCount	= 0;
	wxStopWatch	swTotCmd;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	std::vector<uint8_t> responseBuffer;
	swTotCmd.Start(0);

	while (!Success && m_Running) {
		//STATE_WRITE
		if (!m_CommPort.Write(&vStep, length, 1000)) {
			LogMe("Hardware error or timeout while writing.\n", true);
			m_Running = false; // Interrompiamo l'esecuzione in caso di guasto hardware persistente
			break;
		}
		//LogMe(wxString::Format("Attempt %d,", ++retryCount), false);

		::wxYield();	//Required! (For ask Panic's Button)

		if (!m_Running) break;
		//STATE_READ
		size_t bytesRead = m_CommPort.Read(responseBuffer, vStep.m_Cmd, TimeoutMs);
		if (bytesRead >= sizeof(AnswerHeader)) {
			const AnswerHeader* ptrHeader = reinterpret_cast<const AnswerHeader*>(responseBuffer.data());
			if (ptrHeader) {
				if (m_ptrAnswerShow)	m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);

				Success = (ptrHeader->m_Result == eCmdOk);
				if (vStep.m_Cmd != ptrHeader->m_Cmd) {
					LogMe(wxString::Format("\tAAA: Answer non coherent %d != %d\n", (int)vStep.m_Cmd, (int)ptrHeader->m_Cmd), false);
					Success = false;
				}
				//LogMe(wxString::Format("Answer is %s ...", Success ? "True\n" : "False"), false);
			}
		} else {
			//STATE_ERROR
			LogMe(wxString::Format("Timeout expired (%ld ms). Retransmitting...\n", TimeoutMs), true);
			if (retryCount > 50) {	// Opzionale: aggiungi un limite massimo di tentativi per evitare loop infiniti
				LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
				m_Running = false;	//Stop Execution
				break;
			}
		}
	}
	//STATE_SUCCESS
	LogMe(wxString::Format("  Completed in %ld ms.\n", swTotCmd.Time()), false);
}
#endif

bool cExecutor::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(uint16_t, sCommand&)> onStepCallback) {
	int64_t		detailProg = from;
	sCommand	vStep;
	bool		recordFound;
	cDBSampler	yy(SQLLITEDBPATH);
	uint16_t	n = 0;		//ToDo: Error, deve restituire l'indice della 
	do {
		if (!m_Running)	break;
		recordFound = yy.ProgDetail_Select(m_MasterId, detailProg, vStep);
		if (recordFound) {
			if (onStepCallback) onStepCallback(n++, vStep);
			ExecuteStep(vStep);
			detailProg = vStep.m_DetailProg + 1;
		}
	} while (recordFound && m_Running && detailProg <= to);
	return true;
}

bool cExecutor::ExecuteStep(sCommand& vStep) {
	const sSampler_Commands* x = Command_GetByCmd(vStep.m_SubSystem, vStep.m_Cmd, vStep.m_PatLen);
	LogMe(wxString::Format(" Step Mast:%5d, det:%4d, cmd:%3d (%c) %-25s\t", vStep.m_MasterId, vStep.m_DetailProg, vStep.m_Cmd, vStep.m_Cmd, x->Descr), true);

	if (vStep.m_SubSystem == eSystemCmd && (vStep.m_Cmd == 'a')) {	//Check SubRoutine:
		LogMe(wxString::Format("Execute Subroutine %d\n", vStep.m_Par[0]), false);
		ExecuteSteps_FromDB(vStep.m_Par[0]);
		return true;
	}

	//Check if value come from DB
	std::unique_ptr<cDBSampler> dbPtr = nullptr;
	for (size_t i = 0; i < vStep.m_PatLen; ++i) {
		if (vStep.m_Pattern[i] == 'S') {
			int	iVal = vStep.m_Par[i];
			if (iVal >= 50000) {
				if (!dbPtr)	dbPtr = std::make_unique<cDBSampler>(SQLLITEDBPATH);
				vStep.m_Par[i] = dbPtr->Defaults_NazSteps(iVal - 50000);
			}
		}
	}

	//Add CheckSum
	vStep.m_ChkSum = add_checksum_fast(
		reinterpret_cast<const uint8_t*>(&vStep),
		sizeof(vStep) - sizeof(vStep.m_ChkSum)
	);

#if defined(USE_FSA_EXEC)
	while (!StartSendCommand(vStep, sizeof(vStep))) {
		::wxYield();
		//LogMe(wxString::Format("Stato:%s\n", StateName(m_CmdState)), false);
	}
#else
	SendCommand(vStep, sizeof(vStep));
#endif
	return true;
}

void cExecutor::ExecuteStepSingle(sCommand& vStep) {
	IsRunningSet(true);
	ExecuteStep(vStep);
#if defined(USE_FSA_EXEC)
	while (m_CmdState == STATE_IDLE)	::wxYield();	//Wait starting
	while (m_CmdState != STATE_IDLE)	::wxYield();	//Wait end
	//ToDo: you must execute at least 2 instructions!!!!!!
#endif
	IsRunningSet(false);
}

int cExecutor::GetMotorSelected(void) {
	return (m_ptrAnswerShow ? m_ptrAnswerShow->GetMotorSelected() : -1);
}

void cExecutor::OnTimer(wxTimerEvent&) {
	m_Timer.Stop();
#if defined(USE_FSA_EXEC)
	UpdateSendCommand();
#endif
	bool isReady = IsWorking();
	bool Running = IsRunning();

	if (isReady && m_PoolMotors && !Running) {
		if (m_RotatePool) {
			IncPoolIdx();
		}
		sCommand AskMotor{};// = { eTypCommand, eStepDirect, '0', 1, "M", {m_PoolIdx}, 0, 0, 0 };
		AskMotor.m_MsgType		= eTypCommand;
		AskMotor.m_SubSystem	= eStepDirect;
		AskMotor.m_Cmd			= '0';
		AskMotor.m_PatLen		= 1;
		AskMotor.m_Pattern[0]	= 'M';
		AskMotor.m_Par[0]		= m_PoolIdx;
		AskMotor.m_MasterId		= 0;			//2
		AskMotor.m_DetailProg	= 0;			//2
		AskMotor.m_ChkSum		= 0;			//2

		IsRunningSet(true);
		ExecuteStep(AskMotor);
		IsRunningSet(false);
	}

	m_Timer.Start(250);	// Restart timer
}
