
/*
[Pubblic]
ExecuteSteps_FromDB ────(FetchStepsFromDatabase)──────> [Private] ProcessExecutionQueue ─(FetchStepsFromDatabase)─> SureCommandFromDB ──> SendCommand
                                                    ^
                  ExecuteStep ──────────────────────┴
                      ^
 ExecuteStepSingle ───┘
--------------------------------------------------------------
	
*/

#include "stdwx.h"
#include <wx/stopwatch.h>
#include <array>
#include "cExecutor.h"
#include "cExecTools.h"

// Definizione dell'evento personalizzato
wxDEFINE_EVENT(wxEVT_EXEC_PROMPT_USER, wxCommandEvent);

//#define ChangeStatus(s)	{LogMe(wxString::Format("\tStatus FROM %s --> %s \n", StateName(m_State), StateName(s)), false);	m_State = s; }
#define ChangeStatus(s)	{m_State = s; }


wxString cFsaExec::ExecStateName(eExecutorState s) {
	switch (s) {
#define X(estatus) 	case estatus: return #estatus;
		FSA_STATUS_EXEC_LIST
#undef X
	}
	return "???";
}

#if defined(USE_CFSA_TIMED)
void cFsaExec::OnExecTimer(wxTimerEvent& WXUNUSED(event)) {
	if (m_ptrRunning && m_ptrRunning->load(std::memory_order_relaxed)) 
		fsaExecutor();
}
#endif

cFsaExec::cFsaExec() : m_CommPort()
#if defined(USE_CFSA_TIMED)
, m_execTimer(this)
#endif
{
#if defined(USE_CFSA_TIMED)
	this->Bind(wxEVT_TIMER, &cFsaExec::OnExecTimer, this, m_execTimer.GetId());
	m_execTimer.Start(15);	// millisecond interval
#endif
	m_ResponseBuffer.reserve(sizeof(AnswerHeader) + 2);
}

cFsaExec::~cFsaExec() {
#if defined(USE_CFSA_TIMED)
	m_execTimer.Stop();
#endif
	if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
}

eCmdAnswer cFsaExec::GetAndShowMessage(uint8_t chk, long TimeoutMs) {
	m_ResponseBuffer.clear();
	size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, chk, TimeoutMs);	//Aspetta una risposta al messaggio chk
	if (bytesRead == 0)						return eErrTimeout;
	if (bytesRead < sizeof(AnswerHeader))	return eErrShortMsg;
	const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
	if (!ptrHeader)							return eErrTimeout; //ToDo: maybe never happen
	if (m_ptrAnswerShow)	m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);
	return ptrHeader->m_Result;
}

bool cFsaExec::SendCommand(const sCommand& vStep, long TimeoutMs) {
	if (m_State != EXECSTATE_IDLE)		return false;
	m_vStep = vStep;
	m_TimeoutMs = (TimeoutMs <= 0) ? 500 : TimeoutMs;
	ChangeStatus(EXECSTATE_INITSEND);
	return true;
}

// Questo metodo viene chiamato asincronamente dalla UI (Main Thread) quando l'utente clicca SI o NO
void cFsaExec::PostUserResponse(bool continueProcess) {
	if (m_State == EXECSTATE_WAIT_USER) {
		if (continueProcess) {
			LogMe("\tUSER DECISION: CONTINUE\n", false);
			ChangeStatus(EXECSTATE_ENDSEND);
		} else {
			LogMe("\tUSER DECISION: END PROCESS\n", false);
			if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
			ChangeStatus(EXECSTATE_IDLE);
		}
	}
}

void cFsaExec::fsaExecutor() {
	bool isRunning = m_ptrRunning && m_ptrRunning->load(std::memory_order_relaxed);

	if (!isRunning && m_State != EXECSTATE_IDLE) {
		ChangeStatus(EXECSTATE_IDLE);
		return;
	}

	switch (m_State) {
		case EXECSTATE_INITSEND:
			m_retryCount = 0;
			m_Error = EXEC_ERROR_NONE;
			m_swTotCmd.Start(0);
			ChangeStatus(EXECSTATE_TRYSEND);
			break;

		case EXECSTATE_TRYSEND:
			if (!m_CommPort.Write(&m_vStep, sizeof(sCommand), 1000)) {
				m_Error = EXEC_ERROR_WRITE;
				ChangeStatus(EXECSTATE_ERROR);
				break;
			}
			if (m_retryCount > 50) { m_Error = EXEC_ERROR_TOOMANY;	ChangeStatus(EXECSTATE_ERROR);	break; }
			m_Answ = GetAndShowMessage(m_vStep.m_Cmd, m_TimeoutMs);
			switch (m_Answ) {
				case eCmdOk:										ChangeStatus(EXECSTATE_ENDSEND);	break;
				case eCmdRetry:										ChangeStatus(EXECSTATE_TRYSEND);	break;
				case eCmdError:		m_Error = EXEC_ERROR_ASK;		ChangeStatus(EXECSTATE_ERROR);		break;
				case eErrTimeout:
				case eErrShortMsg:									ChangeStatus(EXECSTATE_CHKCNT);		break;
				default:			m_Error = EXEC_ERROR_UNKNOWN;	ChangeStatus(EXECSTATE_ERROR);		break;
			}
			break;

		case EXECSTATE_ENDSEND:
			tool_LogStep(m_vStep, true);
			LogMe(wxString::Format("  %d/%d Completed in %ld ms.\n", m_vStep.m_MasterId, m_vStep.m_DetailProg, m_swTotCmd.Time()), false);
			ChangeStatus(EXECSTATE_IDLE);
			break;

		case EXECSTATE_CHKCNT:
			m_retryCount++;
			LogMe(wxString::Format("Timeout/ShortMsg (%ld ms). Retransmitting (%d/50)...\n", m_TimeoutMs, m_retryCount), true);
			if (m_retryCount > 50) {
				m_Error = EXEC_ERROR_TIMEOUT;
				ChangeStatus(EXECSTATE_ERROR);
			} else {
				ChangeStatus(EXECSTATE_TRYSEND); // Riprova
			}
			break;

		case EXECSTATE_ERROR:
			switch (m_Error) {
				case EXEC_ERROR_WRITE:
					LogMe("Hardware error or timeout while writing.\n", true);
					if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
					ChangeStatus(EXECSTATE_IDLE);
					break;
				case EXEC_ERROR_ASK:
					LogMe("\tERROR Received. Notifying UI thread...\n", false);
					if (m_parentTarget) {
						wxCommandEvent* evt = new wxCommandEvent(wxEVT_EXEC_PROMPT_USER);	// Creiamo un evento asincrono sicuro da inviare alla UI
						evt->SetClientData(this);			// Passiamo il puntatore di questa istanza nell'evento così la UI sa a chi rispondere
						wxQueueEvent(m_parentTarget, evt);	 //Spedisce l'evento nella coda della UI in modo thread-safe
						ChangeStatus(EXECSTATE_WAIT_USER);	 //Congeliamo la macchina a stati in attesa dell'input utente
					} else {
						// Fallback se nessuno ascolta l'evento: abortiamo per sicurezza
						LogMe("\tNo parent target registered. Aborting.\n", true);
						if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
						ChangeStatus(EXECSTATE_IDLE);
					}
					/*
					LogMe("\tERROR Received. Ask if continue...", false);
					{
						// NOTA: Se fsaExecutor() gira su un Worker Thread secondario, 
						// l'uso di wxMessageDialog sincrono qui causerà crash o deadlock.
						// Deve essere invocato tramite wxQueueEvent o eseguito esclusivamente sul Main Thread.
						wxMessageDialog dialog(nullptr, wxT("ERROR received"), wxT("Continue?"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);
						if (dialog.ShowModal() == wxID_YES) { LogMe("\tCONTINUE\n", false);		ChangeStatus(EXECSTATE_ENDSEND); }
						else { LogMe("\tEND PROCESS\n", false);	*m_ptrRunning = false;	ChangeStatus(EXECSTATE_IDLE); }
					}*/
					break;
				case EXEC_ERROR_TIMEOUT:
				case EXEC_ERROR_TOOMANY:
				case EXEC_ERROR_UNKNOWN:
					if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
					ChangeStatus(EXECSTATE_IDLE);
					break;

				case EXEC_ERROR_SHORTMSG:
					m_retryCount++;
					ChangeStatus(EXECSTATE_TRYSEND);
					break;
#ifdef QQQQQQQQQQQQQQQQQQQQQQQQQQQQQ
				case EXEC_ERROR_TIMEOUT:
					LogMe("\n******* Error: TimeOut. ****\n\n", true);
					m_retryCount++;
					*m_ptrRunning = false;
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
				case EXEC_ERROR_SHORTMSG:
					LogMe("\nShort Message.... retry\n", true);
					m_retryCount++;
					ChangeStatus(EXECSTATE_TRYSEND);
					break;
				case EXEC_ERROR_TOOMANY:
					LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
					*m_ptrRunning = false;
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
				case EXEC_ERROR_UNKNOWN:
					LogMe(wxString::Format("\tERROR: Unknown Result: %d\n", static_cast<int>(m_Answ)), false);
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
#endif
			}
			break;
		case EXECSTATE_WAIT_USER:
			// In questo stato la macchina a stati non fa nulla (gira a vuoto nel timer).
			// Attende che la UI chiami il metodo `PostUserResponse`.
			break;
		case EXECSTATE_IDLE:
		default:
			break;
	}
}

#ifdef HOWHANDLEQQQQQQQ
// Nel costruttore o nella fase di inizializzazione dove crei cFsaQueue/cFsaExec:
m_fsaQueue.m_FsaExec.SetParentTarget(this); // Diciamo all'esecutore di inviare gli eventi a questo Frame

// Colleghiamo l'evento al gestore nel Frame
this->Bind(wxEVT_EXEC_PROMPT_USER, &cMainFrame::OnExecutorPromptUser, this);

// Implementazione del gestore eventi nel Frame (.cpp)
void cMainFrame::OnExecutorPromptUser(wxCommandEvent& event) {
	// Recuperiamo il puntatore all'esecutore che ha generato l'evento
	cFsaExec* executor = static_cast<cFsaExec*>(event.GetClientData());

	if (!executor) return;

	// Mostriamo il dialogo grafico in modo sicuro sul thread della UI
	wxMessageDialog dialog(this, wxT("Si è verificato un errore di comunicazione. Vuoi continuare comunque il processo?"),
		wxT("Errore Esecuzione"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);

	bool userChoice = (dialog.ShowModal() == wxID_YES);

	// Rispondiamo asincronamente alla macchina a stati
	executor->PostUserResponse(userChoice);
}
#endif

//==========================================================================

#if defined(USE_CFSA_TIMEDQ)
void cFsaQueue::OnqueueTimer(wxTimerEvent&) {
	fsaEnqueue();
}
#endif


cFsaQueue::cFsaQueue()
#if defined(USE_CFSA_TIMEDQ)
	: m_queueTimer(this)
#endif
{
#if defined(USE_CFSA_TIMEDQ)
	this->Bind(wxEVT_TIMER, &cFsaQueue::OnqueueTimer, this, m_queueTimer.GetId());
	m_queueTimer.Start(20);	// millisecond interval
#endif
}

cFsaQueue::~cFsaQueue() {
	if (m_ptrQRunning)	m_ptrQRunning->store(false, std::memory_order_seq_cst);
#if defined(USE_CFSA_TIMEDQ)
	m_queueTimer.Stop();
#endif
};

void cFsaQueue::fsaEnqueue() {
	bool isRunning = m_ptrQRunning && m_ptrQRunning->load(std::memory_order_relaxed);

	if (!isRunning) {
		m_enqStatus = ENQUEUE_IDLE;
		return;
	}

	switch (m_enqStatus) {
		case ENQUEUE_IDLE: break;
		case ENQUEUE_LOOP:
			m_enqStatus = (!m_executionQueue.empty()) ? ENQUEUE_CHK : ENQUEUE_IDLE;
			break;
		case ENQUEUE_CHK:
			m_enqStatus = (++m_safetyLoopCounter > MAX_SAFETY_STEPS) ? ENQUEUE_ERROR : ENQUEUE_FETCH;
			break;
		case ENQUEUE_FETCH:
			m_currentStep = tool_Fetch(m_ptrQRunning, m_executionQueue, m_onStepCallback, m_flagShow);
			m_enqStatus = (m_currentStep.m_SubSystem != eSystemCmd || (m_currentStep.m_Cmd != 'a')) ? ENQUEUE_STEP : ENQUEUE_LOOP;
			break;
		case ENQUEUE_STEP:
			if (!m_FsaExec.IsIdle())				break;
			tool_SureCommandFromDB(m_currentStep);	//Eventualmente sostituisce i valori prelevandoli dal DB
			if(m_FsaExec.SendCommand(m_currentStep))	m_enqStatus = ENQUEUE_LOOP;
			break;
		case ENQUEUE_ERROR:
			LogMe("\nCRITICAL ERROR: Overflow nelle subroutine (possible infinito loop in the DB)!\n", true);
			if (m_ptrQRunning) m_ptrQRunning->store(false, std::memory_order_seq_cst);
			m_enqStatus = ENQUEUE_IDLE;
			break;
	}
}

bool cFsaQueue::ProcessExecutionQueue(std::function<void(sCommand&)> onStepCallback, bool flagShow) {
	if (m_enqStatus != ENQUEUE_IDLE)	return false;
	m_safetyLoopCounter	= 0;
	m_onStepCallback	= onStepCallback;
	m_flagShow			= flagShow;
	m_enqStatus			= ENQUEUE_LOOP;

	while (m_enqStatus != ENQUEUE_IDLE)
		::wxYield();

	m_executionQueue.clear();
	return true;
}

bool cFsaQueue::ExecuteStep(sCommand& vStep) {
	if (!m_ptrQRunning || !m_ptrQRunning->load()) return false;

	m_executionQueue.clear();
	m_executionQueue.push_back(vStep);
	return ProcessExecutionQueue(nullptr, false);
}

bool cFsaQueue::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	if (!m_ptrQRunning || !m_ptrQRunning->load()) return false;

	wxStopWatch     swExecFromDB;

	auto initialSteps = tool_FetchStepsFromDB(m_ptrQRunning, m_MasterId, from, to); // Carica i passi iniziali dal database
	if (initialSteps.empty()) return false;

	// Inizializza la coda muovendo direttamente gli elementi dal vettore
	m_executionQueue.clear();
	for (auto& step : initialSteps) {
		m_executionQueue.push_back(std::move(step));
	}

	bool Ret = ProcessExecutionQueue(onStepCallback, false);
	wxString s;
	if (from == 0 && to == wxUINT16_MAX)	s = wxString::Format("Programm %ld took %ld ms.", m_MasterId, swExecFromDB.Time());
	else if (from == 0 && to != wxUINT16_MAX)	s = wxString::Format("Programm %ld (Until %ld) took %ld ms.", m_MasterId, to, swExecFromDB.Time());
	else if (from != 0 && to == wxUINT16_MAX)	s = wxString::Format("Programm %ld (from %d) took %ld ms.", m_MasterId, from, swExecFromDB.Time());
	wxString bar = wxString('*', s.Len() + 8);

	LogMe(wxString::Format("\n%s\n" "*** %s ***\n" "%s\n\n", bar, s, bar), false);

	return Ret;
}

//===================================================================================

cExecutor::cExecutor() : m_Timer(this) {
	m_FsaQueue.SetRunningFlag(m_Running);
	this->Bind(wxEVT_TIMER, &cExecutor::OnTimer, this, m_Timer.GetId());
	m_Timer.Start(200);	// millisecond interval
	m_ResponseBuffer.reserve(sizeof(AnswerHeader)+2);
	m_Running = false;
}

cExecutor::~cExecutor() {
	m_Running = false;
	m_Timer.Stop();
}

bool cExecutor::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	return m_FsaQueue.ExecuteSteps_FromDB(m_MasterId, from, to, onStepCallback);
}

void cExecutor::ExecuteStepSingle(sCommand& step) {
	m_Running = true;
	m_FsaQueue.ExecuteStep(step);
	m_Running = false;
}

void cExecutor::PoolElements() {
	if (m_RotatePool)	IncPoolIdx();

	sCommand AskMotor{};// = { eTypCommand, eStepDirect, '0', 1, "M", {m_PoolIdx}, 0, 0, 0 };
	AskMotor.m_MsgType = eTypCommand;
	AskMotor.m_SubSystem = eStepDirect;
	AskMotor.m_Cmd = '0';
	AskMotor.m_PatLen = 1;
	AskMotor.m_Pattern[0] = 'M';
	AskMotor.m_Par[0] = m_PoolIdx;
	//		AskMotor.m_MasterId		= 0;			//2
	//		AskMotor.m_DetailProg	= 0;			//2
	//		AskMotor.m_ChkSum		= 0;			//2

	ExecuteStepSingle(AskMotor);
}


void cExecutor::OnTimer(wxTimerEvent&) {
	//m_Timer.Stop();
	if (IsWorking() && m_PoolMotors && !m_Running) {
		PoolElements();
	}
	//m_Timer.Start(250);	// Restart timer
}
