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


void cExecutor::SendCommand(const sCommand& vStep, size_t length, long TimeoutMs) {
	bool		Success = false;
	int			retryCount = 0;
	wxStopWatch	swTotCmd;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	std::vector<uint8_t> responseBuffer;
	swTotCmd.Start(0);

	while (!Success && m_Running) {
		if (!m_CommPort.Write(&vStep, length, 1000)) {
			LogMe("Hardware error or timeout while writing.\n", true);
			m_Running = false; // Interrompiamo l'esecuzione in caso di guasto hardware persistente
			break;
		}
		LogMe(wxString::Format(" %d ", ++retryCount), false);
		//::wxYield();

		size_t bytesRead = m_CommPort.Read(responseBuffer, vStep.m_Cmd, TimeoutMs);
		if (bytesRead >= sizeof(AnswerHeader)) {
			const AnswerHeader* ptrHeader = reinterpret_cast<const AnswerHeader*>(responseBuffer.data());
			if (ptrHeader) {
				if (m_ptrAnswerShow)
					m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);

				Success = (ptrHeader->m_Result == eCmdOk);
				if (vStep.m_Cmd != ptrHeader->m_Cmd) {
					LogMe(wxString::Format("\tAAA: Answer non coherent %d != %d\n", (int)vStep.m_Cmd, (int)ptrHeader->m_Cmd), false);
					Success = false;
				}
				LogMe(wxString::Format(" %s ...", Success ? "True\n" : "False"), false);
			}
		} else {
			LogMe(wxString::Format("Timeout scaduto (%ld ms). Ritrasmetto...\n", TimeoutMs), true);
			if (retryCount > 50) {	// Opzionale: aggiungi un limite massimo di tentativi per evitare loop infiniti
				LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
				m_Running = false;	//Stop Execution
				break;
			}
		}
	}
	LogMe(wxString::Format("  Completed in %ld ms.\n", swTotCmd.Time()), false);
}

void cExecutor::ExecuteStepSingle(sCommand& vStep) {
	IsRunning(true);
	ExecuteStep(vStep);
	IsRunning(false);
}

bool cExecutor::ExecuteStep(sCommand& vStep) {
	LogMe("\n\n", false);
	LogMe(wxString::Format("Step %d (%d)\n", vStep.m_DetailProg, vStep.m_Cmd), true);

	//Check SubRoutine:
	if (vStep.m_SubSystem == eSystemCmd && (vStep.m_Cmd == 'a')) {
		LogMe(wxString::Format("Execute Subroutine %d\n", vStep.m_Par[0]), false);
		ExecuteSteps_FromDB(vStep.m_Par[0]);
		return true;
	}

	for (size_t i = 0; i < vStep.m_PatLen; ++i) {
		if (vStep.m_Pattern[i] == 'S') {
			int	iVal = vStep.m_Par[i];
			if (iVal >= 50000) {
				cDBSampler yy(SQLLITEDBPATH);
				vStep.m_Par[i] = yy.Defaults_NazSteps(iVal - 50000);
			}

		}
	}

	vStep.m_ChkSum = add_checksum_fast(
		reinterpret_cast<const uint8_t*>(&vStep),
		sizeof(vStep) - sizeof(vStep.m_ChkSum)
	);

	SendCommand(vStep, sizeof(vStep));

	return true;
}
bool cExecutor::ExecuteSteps_FromDB(uint16_t	m_MasterId) {	//Execute Steps from DB
	int64_t detailProg = 0;
	sCommand vStep;
	bool recordFound;

#if defined(USE_ODBC)
#else
	{
		cDBSampler yy(SQLLITEDBPATH);
		do {
			recordFound = yy.ProgDetail_Select(m_MasterId, detailProg, vStep);
			if (recordFound) {
				ExecuteStep(vStep);
				detailProg = vStep.m_DetailProg + 1;
			}
		} while (recordFound && m_Running );
	}
#endif

	return true;
}

int cExecutor::GetMotorSelected(void) {
	return (m_ptrAnswerShow ? m_ptrAnswerShow->GetMotorSelected() : -1);
}

void cExecutor::IncPoolIdx(void) { m_PoolIdx = (m_PoolIdx + 1) % 3; }    //AAA count Motors

void cExecutor::OnTimer(wxTimerEvent&) {
	m_Timer.Stop();
	bool isReady = IsWorking();
	bool Running = IsRunning();

	if (isReady && m_PoolMotors && !Running) {
		if (m_RotatePool) {
			IncPoolIdx();
		}
		sCommand AskMotor{};// = { eTypCommand, eStepDirect, '0', 1, "M", {m_PoolIdx}, 0, 0, 0 };
		AskMotor.m_MsgType = eTypCommand;
		AskMotor.m_SubSystem = eStepDirect;
		AskMotor.m_Cmd = '0';
		AskMotor.m_PatLen = 1;
		AskMotor.m_Pattern[0] = 'M';
		AskMotor.m_Par[0] = m_PoolIdx;
		AskMotor.m_MasterId = 0;			//2
		AskMotor.m_DetailProg = 0;			//2
		AskMotor.m_ChkSum = 0;			//2

		IsRunning(true);
		ExecuteStep(AskMotor);
		IsRunning(false);
		
		//::wxYield();
	}

	m_Timer.Start(250);	// Restart timer
}
