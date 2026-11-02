#include "stdwx.h"
#include <wx/stopwatch.h>
#include <array>
#include "cExecutor.h"
#include "cExecTools.h"

wxDEFINE_EVENT(wxEVT_EXEC_PROMPT_USER,	wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FSAEXEC_ANSWER,	wxCommandEvent);

void cFsaExec::ChangeStatus(eExecutorState newState) {
#ifdef USE_LOG_CHANGESTATUS // Abilita il log esteso solo in modalità Debug per non appesantire la seriale in Release
	if (m_State != newState && m_EventTarget) {
		tool_LogMe(m_EventTarget, wxString::Format("\tFSM: %s --> %s\n", ExecStateName(m_State), ExecStateName(newState)), false);
	}
#endif
	m_State = newState;
}

wxString cFsaExec::ExecStateName(eExecutorState s) {
	switch (s) {
#define X(estatus) 	case estatus: return #estatus;
		FSA_STATUS_EXEC_LIST
#undef X
	}
	return "???";
}

void cFsaExec::OnExecTimer(wxTimerEvent& WXUNUSED(event)) {
	if (m_ptrRunning && m_ptrRunning->load(std::memory_order_relaxed)) 
		fsaExecutor();
}

cFsaExec::cFsaExec() : m_CommPort(), m_execTimer(this) {
	this->Bind(wxEVT_TIMER, &cFsaExec::OnExecTimer, this, m_execTimer.GetId());
	m_execTimer.Start(15);	// millisecond interval
	m_ResponseBuffer.reserve(sizeof(AnswerHeader) + 2);
}

cFsaExec::~cFsaExec() {
	this->Unbind(wxEVT_TIMER, &cFsaExec::OnExecTimer, this, m_execTimer.GetId());
	m_execTimer.Stop();
	if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
}

eCmdAnswer cFsaExec::GetAndShowMessage(uint8_t chk, long TimeoutMs) {
	m_ResponseBuffer.clear();
	size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, chk, TimeoutMs);	//Aspetta una risposta al messaggio chk
	if (bytesRead == 0)						return eErrTimeout;
	if (bytesRead < sizeof(AnswerHeader))	return eErrShortMsg;
	const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
	if (!ptrHeader)							return eErrTimeout; //ToDo: maybe never happen

	if (m_EventTarget) {
		sFsaExecParams* answ = new sFsaExecParams();
		answ->lenAnswer	= bytesRead;
		answ->Response	= m_ResponseBuffer;
		wxCommandEvent* event = new wxCommandEvent(wxEVT_FSAEXEC_ANSWER, ID_EXEC_ANSWER);
		event->SetClientData(static_cast<void*>(answ));
		wxQueueEvent(m_EventTarget, event);
	}
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
			tool_LogMe(m_EventTarget, "\tUSER DECISION: CONTINUE\n", false);
			ChangeStatus(EXECSTATE_ENDSEND);
		} else {
			tool_LogMe(m_EventTarget, "\tUSER DECISION: END PROCESS\n", false);
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
				case eErrTimeout:	m_Error = EXEC_ERROR_TIMEOUT;	ChangeStatus(EXECSTATE_ERROR);		break;
				case eErrShortMsg:									ChangeStatus(EXECSTATE_CHKCNT);		break;
				default:			m_Error = EXEC_ERROR_UNKNOWN;	ChangeStatus(EXECSTATE_ERROR);		break;
			}
			break;

		case EXECSTATE_ENDSEND:
			tool_LogMe(m_EventTarget, tool_LogStepStr(m_vStep, true), true);
			tool_LogMe(m_EventTarget, wxString::Format("  %d/%d Completed in %ld ms.\n", m_vStep.m_MasterId, m_vStep.m_DetailProg, m_swTotCmd.Time()), false);

			ChangeStatus(EXECSTATE_IDLE);
			break;

		case EXECSTATE_CHKCNT:
			m_retryCount++;
			tool_LogMe(m_EventTarget, wxString::Format("Timeout/ShortMsg (%ld ms). Retransmitting (%d/50)...\n", m_TimeoutMs, m_retryCount), true);
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
					tool_LogMe(m_EventTarget, "Hardware error or timeout while writing.\n", true);
					if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
					ChangeStatus(EXECSTATE_IDLE);
					break;
				case EXEC_ERROR_ASK:
					tool_LogMe(m_EventTarget, "\tERROR Received. Notifying UI thread...\n", false);

					if (m_EventTarget) {
						wxCommandEvent* evt = new wxCommandEvent(wxEVT_EXEC_PROMPT_USER);	// Creiamo un evento asincrono sicuro da inviare alla UI
						evt->SetClientData(this);			// Passiamo il puntatore di questa istanza nell'evento così la UI sa a chi rispondere
						wxQueueEvent(m_EventTarget, evt);	 //Spedisce l'evento nella coda della UI in modo thread-safe
						ChangeStatus(EXECSTATE_WAIT_USER);	 //Congeliamo la macchina a stati in attesa dell'input utente
					} else {
						// Fallback se nessuno ascolta l'evento: abortiamo per sicurezza
						tool_LogMe(m_EventTarget, "\tNo parent target registered. Aborting.\n", true);
						if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
						ChangeStatus(EXECSTATE_IDLE);
					}
					break;
				case EXEC_ERROR_TIMEOUT:
					tool_LogMe(m_EventTarget, "\n********TIMEOUT.\n**********\n", true);
				case EXEC_ERROR_TOOMANY:
				case EXEC_ERROR_UNKNOWN:
					if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_seq_cst);
					ChangeStatus(EXECSTATE_IDLE);
					break;

				case EXEC_ERROR_SHORTMSG:
					m_retryCount++;
					ChangeStatus(EXECSTATE_TRYSEND);
					break;
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

//==========================================================================

void cFsaQueue::OnqueueTimer(wxTimerEvent&) {
	fsaEnqueue();
}

cFsaQueue::cFsaQueue() : m_queueTimer(this) {
	this->Bind(wxEVT_TIMER, &cFsaQueue::OnqueueTimer, this, m_queueTimer.GetId());
	m_queueTimer.Start(20);	// millisecond interval
}

cFsaQueue::~cFsaQueue() {
	this->Unbind(wxEVT_TIMER, &cFsaQueue::OnqueueTimer, this, m_queueTimer.GetId());
	m_queueTimer.Stop();
	if (m_ptrQRunning) m_ptrQRunning->store(false, std::memory_order_seq_cst);
};

void cFsaQueue::fsaEnqueue() {
	bool isRunning = m_ptrQRunning && m_ptrQRunning->load(std::memory_order_relaxed);

	if (!isRunning) {
		m_enqStatus = ENQUEUE_IDLE;
		return;
	}

	bool muoviMacchina = true;
	while (muoviMacchina) {
		muoviMacchina = false; // Di base ci si ferma, a meno che uno stato non richieda un salto immediato

		switch (m_enqStatus) {
			case ENQUEUE_IDLE: break;
			case ENQUEUE_LOOP:
				if (m_executionQueue.empty()) {
					m_enqStatus = ENQUEUE_IDLE;
				} else {
					m_enqStatus = ENQUEUE_CHK;
					muoviMacchina = true; // Continua subito nello stesso scatto del timer
				}
				break;
			case ENQUEUE_CHK:
				if (++m_safetyLoopCounter > MAX_SAFETY_STEPS) {
					m_enqStatus = ENQUEUE_ERROR;
				} else {
					m_enqStatus = ENQUEUE_FETCH;
					muoviMacchina = true; // Continua subito nello stesso scatto del timer
				}
				break;
			case ENQUEUE_FETCH:
				m_currentStep = tool_Fetch();
				m_enqStatus = (m_currentStep.m_SubSystem != eSystemCmd || (m_currentStep.m_Cmd != 'a')) ? ENQUEUE_STEP : ENQUEUE_LOOP;
				muoviMacchina = true; // Continua subito (se è una subroutine siriesegue il loop, se è uno step va a verificare l'hardware)
				break;
			case ENQUEUE_STEP:
				if (!m_FsaExec.IsIdle())				break;
				tool_SureCommandFromDB(m_currentStep);	//Eventualmente sostituisce i valori prelevandoli dal DB
				if (m_FsaExec.SendCommand(m_currentStep)) {
					m_enqStatus = ENQUEUE_LOOP;
					muoviMacchina = true; // Il comando è partito, controlla subito se c'è altro da fare!
				}
				break;
			case ENQUEUE_ERROR:
				tool_LogMe(m_EventTarget, "\nCRITICAL ERROR: Overflow in subroutines (possible infinite loop in the DB)!\n", true);
				if (m_ptrQRunning) m_ptrQRunning->store(false, std::memory_order_seq_cst);
				m_enqStatus = ENQUEUE_IDLE;
				break;
		}
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

sCommand cFsaQueue::tool_Fetch() {
	if (m_executionQueue.empty()) {
		tool_LogMe(m_EventTarget, "ERROR: tool_Fetch chiamata su una coda di esecuzione vuota!", true);
		return sCommand{}; // Ritorna un comando vuoto di default
	}
	// Estrae l'elemento in testa usando lo spostamento (std::move) per massimizzare le prestazioni
	sCommand currentStep = std::move(m_executionQueue.front());
	m_executionQueue.pop_front();

	if (m_flagShow) { // Esegue la notifica sulla callback per l'interfaccia se presente
		if (m_onStepCallback) m_onStepCallback(currentStep);
		tool_LogMe(m_EventTarget, tool_LogStepStr(currentStep, false), true);
	}

	// Il comando estratto è una chiamata a una sotto-routine
	if (currentStep.m_SubSystem == eSystemCmd && (currentStep.m_Cmd == 'a')) {
		tool_LogMe(m_EventTarget, wxString::Format("-> Espansione SubRoutine %d...\n", currentStep.m_Par[0]), false);
		auto subSteps = tool_FetchStepsFromDB(m_ptrQRunning, currentStep.m_Par[0], 0, wxUINT16_MAX);

		// Inserisce i nuovi passi in TESTA alla coda in ordine inverso (LIFO)
		for (auto it = subSteps.rbegin(); it != subSteps.rend(); ++it) {
			m_executionQueue.push_front(std::move(*it));
		}
	}
	return currentStep;
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

	tool_LogMe(m_EventTarget, wxString::Format("\n%s\n" "*** %s ***\n" "%s\n\n", bar, s, bar), false);
	return Ret;
}

//===================================================================================
cExecutor::cExecutor() : m_FsaQueue() {
	m_FsaQueue.SetRunningFlag(m_Running);
	m_Running = false;
	m_poller = std::make_unique<cMotorPoller>(this);
	m_poller->Start(200); // Fa partire il timer interno del poller
}

cExecutor::~cExecutor() {
	m_Running = false;
}

bool cExecutor::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	return m_FsaQueue.ExecuteSteps_FromDB(m_MasterId, from, to, onStepCallback);
}

void cExecutor::ExecuteStepSingle(sCommand& step) {
	m_Running = true;
	m_FsaQueue.ExecuteStep(step);
	m_Running = false;
}

void cExecutor::StartProcess(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	IsRunningSet(true);
	ExecuteSteps_FromDB(m_MasterId, from, to, onStepCallback);
	IsRunningSet(false);
}