#include "stdwx.h"
#include <wx/stopwatch.h>
#include <array>
#include "cExecutor.h"
#include "cExecTools.h"

/*
ExecuteStepSingle	--> ExecuteStep				-->	ProcessExecutionQueue
StartProcess		--> ExecuteSteps_FromDB		-->	ProcessExecutionQueue
*/


wxDEFINE_EVENT(wxEVT_EXEC_PROMPT_USER,	wxCommandEvent);	//cFsaExec
wxDEFINE_EVENT(wxEVT_FSAEXEC_ANSWER,	wxCommandEvent);	//eCmdAnswer
wxDEFINE_EVENT(wxEVT_QUEUE_COMPLETED,	wxCommandEvent);

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

cFsaExec::cFsaExec() : m_CommPort() {
	m_ResponseBuffer.reserve(sizeof(AnswerHeader) + 2);
	m_State = EXECSTATE_IDLE;
}

cFsaExec::~cFsaExec() {
	if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_relaxed);
}

eCmdAnswer cFsaExec::GetAndShowMessage(uint8_t chk, long TimeoutMs) {
	m_ResponseBuffer.clear();
	//ToDo: Rimuovere il check di chk da m_CommPort.Read: deve uscire subito
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
		m_State = EXECSTATE_IDLE;
		return;
	}

	switch (m_State) {
		case EXECSTATE_INITSEND:
			m_retryCount = 0;
			m_Error = EXEC_ERROR_NONE;
			m_swTotCmd.Start(0); // Avvia il cronometro per il calcolo del timeout
			m_State = EXECSTATE_TRYSEND;
			// Fall-through intenzionale per inviare il pacchetto senza aspettare altri 20ms
			[[fallthrough]];

		case EXECSTATE_TRYSEND:
			// Scrittura fisica sulla porta seriale
			if (!m_CommPort.Write(&m_vStep, sizeof(sCommand), 1000)) {
				m_Error = EXEC_ERROR_WRITE;
				m_State = EXECSTATE_ERROR;
				break;
			}
			m_State = EXECSTATE_GETANSW;
			break;

		case EXECSTATE_GETANSW:
			// Verifica se sono arrivati i dati dalla seriale (in modalità non bloccante)
			m_Answ = GetAndShowMessage(m_vStep.m_Cmd, 30); // ToDo: Era 0... Passiamo 0ms per non bloccare il Main Thread!

			switch (m_Answ) {
				case eCmdOk:
					m_State = EXECSTATE_ENDSEND;
					break;
				case eCmdRetry:
					m_State = EXECSTATE_TRYSEND; // Il firmware della scheda chiede di riprovare l'invio
					break;
				case eCmdError:
					m_Error = EXEC_ERROR_ASK;
					m_State = EXECSTATE_ERROR;
					break;
				case eErrTimeout:
				case eErrShortMsg:
					// Se non è arrivato nulla o il messaggio è parziale, verifichiamo il cronometro software
					if (m_swTotCmd.Time() > m_TimeoutMs) {
						m_State = EXECSTATE_CHKCNT; // Tempo scaduto, gestiamo il tentativo di retransmitting
					}
					// Se il tempo NON è scaduto, la macchina resta in EXECSTATE_GETANSW 
					// e ricontrollerà la seriale al prossimo impulso della coda (tra 20 ms).
					break;
				default:
					m_Error = EXEC_ERROR_UNKNOWN;
					m_State = EXECSTATE_ERROR;
					break;
			}
			break;

		case EXECSTATE_ENDSEND:
			tool_LogMe(m_EventTarget, tool_LogStepStr(m_vStep, true), true);
			tool_LogMe(m_EventTarget, wxString::Format("  %d/%d Completed in %ld ms.\n", m_vStep.m_MasterId, m_vStep.m_DetailProg, m_swTotCmd.Time()), false);
			m_State = EXECSTATE_IDLE;
			break;

		case EXECSTATE_CHKCNT:
			m_retryCount++;
			tool_LogMe(m_EventTarget, wxString::Format("Timeout/ShortMsg (%ld ms). Retransmitting (%d/50)...\n", m_TimeoutMs, m_retryCount), true);
			if (m_retryCount > 50) {
				m_Error = EXEC_ERROR_TIMEOUT;
				m_State = EXECSTATE_ERROR;
			}
			else {
				m_swTotCmd.Start(0); // Resetta il cronometro per il nuovo tentativo
				m_State = EXECSTATE_TRYSEND; // Riprova a inviare fisicamente
			}
			break;

		case EXECSTATE_ERROR:
			switch (m_Error) {
			case EXEC_ERROR_WRITE:
				tool_LogMe(m_EventTarget, "Hardware error or timeout while writing.\n", true);
				if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_relaxed);
				m_State = EXECSTATE_IDLE;
				break;
			case EXEC_ERROR_ASK:
				tool_LogMe(m_EventTarget, "\tERROR Received. Notifying UI thread...\n", false);
				if (m_EventTarget) {
					wxCommandEvent* evt = new wxCommandEvent(wxEVT_EXEC_PROMPT_USER);
					evt->SetClientData(this); // Passa l'istanza per permettere la PostUserResponse
					wxQueueEvent(m_EventTarget, evt);
					m_State = EXECSTATE_WAIT_USER; // Congela l'esecutore in attesa dell'utente
				}
				else {
					tool_LogMe(m_EventTarget, "\tNo parent target registered. Aborting.\n", true);
					if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_relaxed);
					m_State = EXECSTATE_IDLE;
				}
				break;
			default:
				if (m_ptrRunning) m_ptrRunning->store(false, std::memory_order_relaxed);
				m_State = EXECSTATE_IDLE;
				break;
			}
			break;

		case EXECSTATE_WAIT_USER:
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
		if (m_enqStatus != ENQUEUE_IDLE) {
			m_executionQueue.clear();
			m_enqStatus = ENQUEUE_IDLE;
		}
		return;
	}

	// 1. GESTIONE DELL'ESECUTORE BASSO (AD IMPULSI):
	// Se l'esecutore hardware sta attendendo una risposta dalla seriale (EXECSTATE_GETANSW) 
	// o sta gestendo un errore/reinvio, gli diamo un "impulso" usando il timer della coda (ogni 20ms).
	if (!m_FsaExec.IsIdle()) {
		m_FsaExec.fsaExecutor(); // Controlla la porta, valuta il timeout software, ecc.

		// Finché l'esecutore non torna in IDLE, congeliamo l'avanzamento della coda 
		// e ritorniamo al ciclo dei messaggi di wxWidgets.
		return;
	}

	// 2. CICLO DI AVANZAMENTO DELLA CODA (OTTIMIZZATO IN MEMORIA):
	// Usiamo un flag per processare gli stati di memoria in cascata senza aspettare 20ms per ogni cambio stato
	bool stateChanged = true;
	while (stateChanged) {
		stateChanged = false;

		switch (m_enqStatus) {
		case ENQUEUE_IDLE:
			break;

		case ENQUEUE_LOOP:
			if (m_executionQueue.empty()) {
				m_enqStatus = ENQUEUE_IDLE;

				// La coda è terminata! Lanciamo l'evento asincrono per svegliare la GUI
				/*if (m_EventTarget) {
					wxCommandEvent* completionEvent = new wxCommandEvent(wxEVT_QUEUE_COMPLETED);
					wxQueueEvent(m_EventTarget, completionEvent);
				}*/
				if (m_EventCompleted) {
					wxCommandEvent* completionEvent = new wxCommandEvent(wxEVT_QUEUE_COMPLETED);
					wxQueueEvent(m_EventCompleted, completionEvent);
				}
			}
			else {
				m_enqStatus = ENQUEUE_CHK;
				stateChanged = true; // Salta subito al controllo di sicurezza
			}
			break;

		case ENQUEUE_CHK:
			if (++m_safetyLoopCounter > MAX_SAFETY_STEPS) {
				m_enqStatus = ENQUEUE_ERROR;
			}
			else {
				m_enqStatus = ENQUEUE_FETCH;
				stateChanged = true; // Salta subito all'estrazione del comando
			}
			break;

		case ENQUEUE_FETCH:
			m_currentStep = tool_Fetch();

			// Se è un comando normale va a ENQUEUE_STEP, se è un'espansione subroutine torna a LOOP
			m_enqStatus = (m_currentStep.m_SubSystem != eSystemCmd || (m_currentStep.m_Cmd != 'a')) ? ENQUEUE_STEP : ENQUEUE_LOOP;
			stateChanged = true; // Continua l'elaborazione immediata dello stato scelto
			break;

		case ENQUEUE_STEP:
			// Controllo di sicurezza ridondante (m_FsaExec è già in IDLE grazie al controllo in cima)
			tool_SureCommandFromDB(m_currentStep);	// Sostituisce i valori prelevandoli dal DB

			if (m_FsaExec.SendCommand(m_currentStep)) {
				// Abbiamo avviato il comando hardware (m_FsaExec ora è in EXECSTATE_INITSEND).
				// Forziamo IMMEDIATAMENTE il primo impulso esecutivo per inviare il pacchetto 
				// sulla porta seriale senza attendere il prossimo scatto del timer da 20ms.
				m_FsaExec.fsaExecutor();

				// Impostiamo lo stato successivo e torniamo al ciclo principale. 
				// Al prossimo scatto del timer, la funzione si bloccherà sul controllo "if (!m_FsaExec.IsIdle())"
				// finché la transazione hardware (o il timeout) non sarà completata.
				m_enqStatus = ENQUEUE_LOOP;
			}
			break;

		case ENQUEUE_ERROR:
			tool_LogMe(m_EventTarget, "\nCRITICAL ERROR: Overflow nelle subroutine (possible infinito loop in the DB)!\n", true);
			if (m_ptrQRunning) m_ptrQRunning->store(false, std::memory_order_seq_cst);
			m_executionQueue.clear();
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

	return true;
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

bool cFsaQueue::fillQueue(sCommand& vStep) {
	// Verifica preventiva dello stato di esecuzione
	if (!m_ptrQRunning || !m_ptrQRunning->load(std::memory_order_relaxed)) {
		return false;
	}

	m_executionQueue.clear();
	m_executionQueue.push_back(std::move(vStep)); // Usa std::move per efficienza
	return true;
}

bool cFsaQueue::fillQueue(uint16_t m_MasterId, uint16_t from, uint16_t to) {
	if (!m_ptrQRunning || !m_ptrQRunning->load(std::memory_order_relaxed)) {
		return false;
	}

	auto initialSteps = tool_FetchStepsFromDB(m_ptrQRunning, m_MasterId, from, to);
	if (initialSteps.empty())	return false;	//Process not found

	// Inizializza la coda muovendo direttamente gli elementi dal vettore
	m_executionQueue.clear();
	for (auto& step : initialSteps) {
		m_executionQueue.push_back(std::move(step));
	}
	return true;
}

bool cFsaQueue::ExecuteStep(sCommand& vStep) {
	if (!fillQueue(vStep))	return false;
	return ProcessExecutionQueue(nullptr, false);
}

bool cFsaQueue::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	if (!fillQueue(m_MasterId, from, to))	return false;
	return ProcessExecutionQueue(onStepCallback, false);
}

//===================================================================================
cExecutor::cExecutor() : m_FsaQueue() {
	m_FsaQueue.SetRunningFlag(m_Running);
	m_Running = false;
#if defined(USENEWEXEC)
	// Invia gli eventi di cFsaQueue a questo cExecutor
	m_FsaQueue.SetEventTarget(this);

	// Agganciamo l'evento di fine coda
	this->Bind(wxEVT_QUEUE_COMPLETED, &cExecutor::OnQueueCompleted, this);
#endif
}

cExecutor::~cExecutor() {
#if defined(USENEWEXEC)
	this->Unbind(wxEVT_QUEUE_COMPLETED, &cExecutor::OnQueueCompleted, this);
#endif
	m_Running = false;
}

void cExecutor::ExecuteStepSingle(sCommand& step) {
#if defined(USENEWEXEC)
	if (m_Running) return; // Protezione: se sta già girando un programma, rifiuta

	IsRunningSet(true);   // Accendiamo il flag operativo
	m_FsaQueue.ExecuteStep(step);
	// NOTA: NON spegniamo m_Running qui! Ci penserà OnQueueCompleted.
#else
	m_Running = true;
	m_FsaQueue.ExecuteStep(step);
	m_Running = false;
#endif
}

void cExecutor::StartProcess(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
#if defined(USENEWEXEC)
	if (m_Running) return; // Protezione

	IsRunningSet(true); // Il sistema è ufficialmente occupato

	// Memorizziamo i dati del contesto per poter stampare il log alla fine
	m_currentMasterId = m_MasterId;
	m_currentFrom = from;
	m_currentTo = to;

	// Far partire il cronometro MEMBRO della classe
	m_swTotalProcess.Start(0);

	bool r = m_FsaQueue.ExecuteSteps_FromDB(m_MasterId, from, to, onStepCallback);

	if (!r) {
		// Se il database era vuoto o il caricamento è fallito, spegniamo subito
		IsRunningSet(false);
	}
	// Ritorna SUBITO al Main Thread. La GUI resta fluida al 100%.
#else
	IsRunningSet(true);

	wxStopWatch     swExecFromDB;
	bool r = m_FsaQueue.ExecuteSteps_FromDB(m_MasterId, from, to, onStepCallback);
	if (r) {
		wxString s;
			 if (from == 0 && to == wxUINT16_MAX)	s = wxString::Format("Programm %ld took %ld ms.",				m_MasterId,			swExecFromDB.Time());
		else if (from == 0 && to != wxUINT16_MAX)	s = wxString::Format("Programm %ld (Until %ld) took %ld ms.",	m_MasterId, to,		swExecFromDB.Time());
		else if (from != 0 && to == wxUINT16_MAX)	s = wxString::Format("Programm %ld (from %d) took %ld ms.",		m_MasterId, from,	swExecFromDB.Time());
		wxString bar = wxString('*', s.Len() + 8);

		tool_LogMe(m_EventTarget, wxString::Format("\n%s\n" "*** %s ***\n" "%s\n\n", bar, s, bar), false);
	}
	IsRunningSet(false);
#endif
}

#if defined(USENEWEXEC)
void cExecutor::OnQueueCompleted(wxCommandEvent& event) {
	// 1. Spegniamo il flag globale: ora il sistema è di nuovo IDLE e pronto ad accettare nuovi comandi o Polling
	IsRunningSet(false);

	// 2. Calcoliamo il tempo REALE impiegato dall'hardware (poteva durare anche diversi secondi/minuti)
	long tempoImpiegatoMs = m_swTotalProcess.Time();

	// 3. Generiamo il log finale accurato (solo se stavamo eseguendo un processo da DB)
	if (m_currentMasterId > 0) {
		wxString s;
		if (m_currentFrom == 0 && m_currentTo == wxUINT16_MAX)
			s = wxString::Format("Programm %ld took %ld ms. (REAL HARDWARE TIME)", m_currentMasterId, tempoImpiegatoMs);
		else if (m_currentFrom == 0 && m_currentTo != wxUINT16_MAX)
			s = wxString::Format("Programm %ld (Until %ld) took %ld ms. (REAL HARDWARE TIME)", m_currentMasterId, m_currentTo, tempoImpiegatoMs);
		else if (m_currentFrom != 0 && m_currentTo == wxUINT16_MAX)
			s = wxString::Format("Programm %ld (from %d) took %ld ms. (REAL HARDWARE TIME)", m_currentMasterId, m_currentFrom, tempoImpiegatoMs);

		wxString bar = wxString('*', s.Len() + 8);
		tool_LogMe(m_EventTarget, wxString::Format("\n%s\n" "*** %s ***\n" "%s\n\n", bar, s, bar), false);

		// Resettiamo il contesto
		m_currentMasterId = 0;
	}

	// 4. Se la finestra principale della GUI deve sapere che l'esecuzione è finita 
	// (es. per riabilitare i pulsanti sullo schermo), lasciamo che l'evento continui a salire
	event.Skip();
}
#endif