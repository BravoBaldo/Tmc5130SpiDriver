
/*
[Pubblic]
ExecuteSteps_FromDB ────(FetchStepsFromDatabase)──────> [Private] ProcessExecutionQueue ─(FetchStepsFromDatabase)─> DoExecuteSingleStep ──> SendCommand
                                                    ^
                  ExecuteStep ──────────────────────┴
                      ^
 ExecuteStepSingle ───┘
*/


#include "stdwx.h"
#include <wx/stopwatch.h>
#include <array>
#include "cExecutor.h"

#if defined(USE_THREAD)
bool cExecutor::StartParallelExecution() {
	// Crea il thread parallelo sfruttando wxThreadHelper
	if (CreateThread(wxTHREAD_DETACHED) != wxTHREAD_NO_ERROR) {
		wxLogError("Impossibile creare il thread di esecuzione parallela!");
		m_Running = false;
		return false;
	}

	// Avvia l'esecuzione del thread (chiama internamente il metodo Entry())
	if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
		wxLogError("Impossibile avviare il thread di esecuzione parallela!");
		m_Running = false;
		return false;
	}

	return true;
}
void cExecutor::StopParallelExecution() {
	m_Running = false;

	// Richiede la terminazione controllata del thread se è ancora attivo
	if (GetThread() && GetThread()->IsRunning()) {
		m_Running = false;	// Segnala al thread di chiudersi (IsAlive controlla internamente)
	}
}
//virtual wxThread::ExitCode Entry();// override;
// L'ENTRY POINT DEL THREAD IN PARALLELO
wxThread::ExitCode cExecutor::Entry() {
	// Il thread gira continuamente finché m_Running è true 
	// e finché wxWidgets non richiede il test di terminazione
	while (m_Running && !GetThread()->TestDestroy()) {
		fsaExecutor();	// 1. Esegui un ciclo della macchina a stati dell'Executor (es. gestione I/O, risposte hardware)
		fsaEnqueue(); // 2. Esegui un ciclo della macchina a stati dell'Enqueue (es. fetch comandi dal DB, scorrimento coda)

		// 3. OTTIMIZZAZIONE CPU CRITICA:
		// Se non inserisci una micro-pausa, questo ciclo while metterà la CPU al 100%.
		// Un delay da 1 a 5 millisecondi riduce l'uso della CPU quasi allo 0% 
		// pur mantenendo una reattività hardware eccezionale (200-1000 controlli al secondo).
		wxMilliSleep(5);
	}
	return (wxThread::ExitCode)0;
}

#endif



cExecutor::cExecutor() : m_CommPort(), m_Timer(this) {
	this->Bind(wxEVT_TIMER, &cExecutor::OnTimer, this, m_Timer.GetId());
	m_Timer.Start(100);	// millisecond interval
	m_ResponseBuffer.reserve(sizeof(AnswerHeader)+2);
	m_Running = false;
}

cExecutor::~cExecutor() {
#if defined(USE_THREAD)
	StopParallelExecution();	// Sicurezza: se l'oggetto viene distrutto, fermiamo prima il thread
#endif
	m_Running = false;
	m_Timer.Stop();
}

eCmdAnswer cExecutor::GetAndShowMessage(uint8_t chk, long TimeoutMs) {
	m_ResponseBuffer.clear(); 
	size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, chk, TimeoutMs);	//Aspetta una risposta al messaggio chk
	if (bytesRead <= 0)						return eErrTimeout;
	if (bytesRead < sizeof(AnswerHeader))	return eErrShortMsg;
	const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
	if (!ptrHeader)							return eErrTimeout; //ToDo: maybe never happen
	if (m_ptrAnswerShow)	m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);
	return ptrHeader->m_Result;
}

void cExecutor::LogStep(const sCommand& vStep) {
	const sSampler_Commands* x = Command_GetByCmd(vStep.m_SubSystem, vStep.m_Cmd, vStep.m_PatLen);
	LogMe(wxString::Format(" Step %5d/%03d, cmd:%3d (%c) %-25s\t",
		vStep.m_MasterId, vStep.m_DetailProg, vStep.m_Cmd, vStep.m_Cmd, x->Descr), true);
}

void cExecutor::SureCommandFromDB(sCommand& vStep) {
	std::unique_ptr<cDBSampler> dbPtr = nullptr;
	for (size_t i = 0; i < vStep.m_PatLen; ++i) {
		if (vStep.m_Pattern[i] == 'S' && vStep.m_Par[i] >= 50000) {
			if (!dbPtr) dbPtr = std::make_unique<cDBSampler>(SQLLITEDBPATH);
			vStep.m_Par[i] = dbPtr->Defaults_NazSteps(vStep.m_Par[i] - 50000);
		}
	}
	vStep.m_ChkSum = add_checksum_fast( reinterpret_cast<const uint8_t*>(&vStep), sizeof(vStep) - sizeof(vStep.m_ChkSum) );
}

wxString cExecutor::StateName(eExecutorState s) {
	switch (s) {
#define X(estatus) 	case estatus: return #estatus;
		FSA_STATUS_EXEC_LIST
#undef X
	}
	return "???";
}

#if defined(USE_FSA_EXEC)

//#define ChangeStatus(s)	{LogMe(wxString::Format("\tStatus FROM %s --> %s \n", StateName(m_State), StateName(s)), false);	m_State = s; }
#define ChangeStatus(s)	{m_State = s; }

bool cExecutor::SendCommand(const sCommand& vStep, long TimeoutMs) {
	while (m_State != EXECSTATE_IDLE) return false;
	m_vStep		= vStep;
	m_TimeoutMs = (TimeoutMs <= 0) ? 500 : TimeoutMs;
	ChangeStatus(EXECSTATE_INITSEND);
	return true;
}

void    cExecutor::fsaExecutor() {
	if (!m_Running && m_State != EXECSTATE_IDLE) {
		ChangeStatus(EXECSTATE_IDLE);
		return;
	}
	switch (m_State) {
		case EXECSTATE_INITSEND:
			m_retryCount	= 0;
			m_Error			= EXEC_ERROR_NONE;
			m_swTotCmd.Start(0);
			ChangeStatus(EXECSTATE_TRYSEND);
			break;

		case EXECSTATE_TRYSEND:
			if (!m_CommPort.Write(&m_vStep, sizeof(sCommand), 1000))	{ m_Error = EXEC_ERROR_WRITE;	ChangeStatus(EXECSTATE_ERROR);	break; }
			if (!m_Running)												{								ChangeStatus(EXECSTATE_IDLE);	break; }
			if (m_retryCount>50)										{ m_Error = EXEC_ERROR_TOOMANY;	ChangeStatus(EXECSTATE_ERROR);	break; }
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
			LogStep(m_vStep);
			LogMe(wxString::Format("  %d/%d Completed in %ld ms.\n", m_vStep.m_MasterId, m_vStep.m_DetailProg, m_swTotCmd.Time()), false);
			ChangeStatus(EXECSTATE_IDLE);
			break;

		case EXECSTATE_CHKCNT:
			m_retryCount++;
			LogMe(wxString::Format("Timeout/ShortMsg (%ld ms). Retransmitting (%d/50)...\n", m_TimeoutMs, m_retryCount), true);
			if (m_retryCount > 50) {
				m_Error = EXEC_ERROR_TIMEOUT;
				ChangeStatus(EXECSTATE_ERROR);
			}
			else {
				ChangeStatus(EXECSTATE_TRYSEND); // Riprova
			}
			break;

		case EXECSTATE_ERROR:
			switch (m_Error) {
				case EXEC_ERROR_WRITE:
					LogMe("Hardware error or timeout while writing.\n", true);
					m_Running = false; // STATE_ERROR_ABORT
					break;
				case EXEC_ERROR_ASK:
					LogMe("\tERROR Received. Ask if continue...", false);
					{
						wxMessageDialog dialog(nullptr, wxT("ERROR received"), wxT("Continue?"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);
						if (dialog.ShowModal() == wxID_YES) { LogMe("\tCONTINUE\n", false);		ChangeStatus(EXECSTATE_ENDSEND); }
						else								{ LogMe("\tEND PROCESS\n", false);	m_Running = false;	ChangeStatus(EXECSTATE_IDLE);	 }
					}
					break;
				case EXEC_ERROR_TIMEOUT:
					LogMe("\n******* Error: TimeOut. ****\n\n", true);
					m_retryCount++;
					m_Running = false;
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
				case EXEC_ERROR_SHORTMSG:
					LogMe("\nShort Message.... retry\n", true);
					m_retryCount++;
					ChangeStatus(EXECSTATE_TRYSEND);
					break;
				case EXEC_ERROR_TOOMANY:
					LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
					m_Running = false;
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
				case EXEC_ERROR_UNKNOWN:
					LogMe(wxString::Format("\tERROR: Unknown Result: %d\n", static_cast<int>(m_Answ)), false);
					ChangeStatus(EXECSTATE_IDLE);	//ToDo: AAA dovrebbe essere bloccante
					break;
			}
	}
}
#endif

#if defined(USE_FSA_EXEC)
bool cExecutor::SendCommandOld(const sCommand& vStep, long TimeoutMs) {
#else
bool cExecutor::SendCommand(const sCommand & vStep, long TimeoutMs) {
#endif
	//EXECSTATE_INITSEND
	bool		Success		= false;
	int			retryCount	= 0;
	wxStopWatch	swTotCmd;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	swTotCmd.Start(0);

	while (Success==false && m_Running==true) {
		//STATE_SEND_COMMAND
		if (!m_CommPort.Write(&vStep, sizeof(sCommand), 1000)) {
			LogMe("Hardware error or timeout while writing.\n", true);
			m_Running = false; // STATE_ERROR_ABORT
			break;
		}

		::wxYield();	//Required! (For ask Panic's Button)

		if (!m_Running) break;

		eCmdAnswer Answ = GetAndShowMessage(vStep.m_Cmd, TimeoutMs);
		switch (Answ) {
			case eCmdRetry:		Success = false;	break;
			case eCmdOk:		Success = true;		break;
			case eCmdError:
				{
					LogMe("\tERROR Received. Ask if continue...", false);
					wxMessageDialog dialog(nullptr, wxT("ERROR received"), wxT("Continue?"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);
					switch (dialog.ShowModal()) {
						case wxID_YES:	LogMe("\tCONTINUE\n", false);		Success = true;	break;
						case wxID_NO:
						default:		LogMe("\tEND PROCESS\n", false);	m_Running = false; return true;
					}
				}
				break;	//Ask if continue
			
			case eErrTimeout:	//	if (bytesRead <= 0)	
			case eErrShortMsg:	//	if (bytesRead < sizeof(AnswerHeader))
				{
					retryCount++;
					LogMe(wxString::Format("Timeout expired (%ld ms). Retransmitting (Attempt %d/50)...\n", TimeoutMs, retryCount), true);
					if (retryCount > 50) {	// Opzionale: aggiungi un limite massimo di tentativi per evitare loop infiniti
						LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
						m_Running = false;
						break;
					}
				}
				break;

			default:	//Error in Code
					LogMe(wxString::Format("\tERROR: Unknown Result: %d\n", static_cast<int>(Answ)), false);
					Success = false;
					break;
		}
	}

	LogStep(vStep);
	LogMe(wxString::Format("  %d/%d Completed in %ld ms.\n", vStep.m_MasterId, vStep.m_DetailProg, swTotCmd.Time()), false);
	return true;
}

bool cExecutor::DoExecuteSingleStep(sCommand& vStep) {
	if (!m_Running) return false;
	SureCommandFromDB(vStep);
	return SendCommand(vStep);
}


sCommand cExecutor::Fetch(std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow) {
	// Estrae l'elemento in testa usando lo spostamento (std::move) per massimizzare le prestazioni
	sCommand currentStep = std::move(executionQueue.front());
	executionQueue.pop_front();

	if (flagShow) {
		// Esegue la notifica sulla callback per l'interfaccia se presente
		if (onStepCallback) onStepCallback(currentStep);
		LogStep(currentStep); // AAA always log!
	}

	// CASO A: Il comando estratto è una chiamata a una sotto-routine
	if (currentStep.m_SubSystem == eSystemCmd && (currentStep.m_Cmd == 'a')) {
		LogMe(wxString::Format("-> Espansione SubRoutine %d...\n", currentStep.m_Par[0]), false);
		auto subSteps = FetchStepsFromDatabase(currentStep.m_Par[0], 0, wxUINT16_MAX);	// Estrae i passi della sotto-routine dal DB

		// Inserisce i nuovi passi in TESTA alla coda in ordine inverso (LIFO)
		for (auto it = subSteps.rbegin(); it != subSteps.rend(); ++it) {
			executionQueue.push_front(std::move(*it));
		}
	}
	return currentStep;
}


#if defined(USE_FSA_QUEUE)
void cExecutor::fsaEnqueue() {
	// Se l'esecuzione è stata arrestata dall'esterno, resettiamo la macchina
	if (!m_Running && m_enqStatus != ENQUEUE_IDLE) {
		m_enqStatus = ENQUEUE_IDLE;
		return;
	}

	switch (m_enqStatus) {
		case ENQUEUE_IDLE: break;
		case ENQUEUE_LOOP:
			m_enqStatus = (!m_ptrExecutionQueue->empty() && m_Running) ? ENQUEUE_CHK : ENQUEUE_IDLE;
			break;
		case ENQUEUE_CHK:
			m_enqStatus = (++m_safetyLoopCounter > MAX_SAFETY_STEPS) ? ENQUEUE_ERROR : ENQUEUE_FETCH;
			break;
		case ENQUEUE_FETCH:
			m_currentStep = Fetch(*m_ptrExecutionQueue, m_onStepCallback, m_flagShow);
			m_enqStatus = (m_currentStep.m_SubSystem != eSystemCmd || (m_currentStep.m_Cmd != 'a')) ? ENQUEUE_STEP : ENQUEUE_LOOP;
			break;
		case ENQUEUE_STEP:
			if (DoExecuteSingleStep(m_currentStep)) m_enqStatus = ENQUEUE_LOOP;
			break;
		case ENQUEUE_ERROR: 
			LogMe("\nCRITICAL ERROR: Overflow nelle subroutine (possible infinito loop in the DB)!\n", true);
			m_Running = false;
			m_enqStatus = ENQUEUE_IDLE;
			break;
	}
}
bool cExecutor::ProcessExecutionQueue(std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow) {
	if (m_enqStatus != ENQUEUE_IDLE)
		return false;
	m_safetyLoopCounter = 0;
	m_ptrExecutionQueue = &executionQueue;
	m_onStepCallback	= onStepCallback;
	m_flagShow			= flagShow;
	m_enqStatus			= ENQUEUE_LOOP;

	while(m_enqStatus != ENQUEUE_IDLE)
		::wxYield();
	return true;
}
#endif

#if defined(USE_FSA_QUEUE)
bool cExecutor::ProcessExecutionQueueOld(std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow) {
#else
bool cExecutor::ProcessExecutionQueue(std::deque<sCommand>&executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow) {
#endif
	size_t safetyLoopCounter = 0;

	while (!executionQueue.empty() && m_Running) {
		if (++safetyLoopCounter > MAX_SAFETY_STEPS) {
			LogMe("\nCRITICAL ERROR: Overflow nelle subroutine (possible infinito loop in the DB)!\n", true);
			return false;
		}

		// Estrae ed espande l'eventuale sotto-routine internamente
		sCommand currentStep = Fetch(executionQueue, onStepCallback, flagShow);

		// Se il comando estratto NON è una macro di sistema, allora è un comando hardware reale
		if (currentStep.m_SubSystem != eSystemCmd || currentStep.m_Cmd != 'a') {
			// OTTIMIZZAZIONE SICUREZZA: Controlliamo m_Running anche nel ciclo interno.
			// Se l'hardware fallisce e l'utente preme "Stop/Annulla", usciamo immediatamente
			// evitando un loop infinito e il congelamento del programma.
			while (!DoExecuteSingleStep(currentStep) && m_Running) {
				::wxYield(); // Cede il controllo alla GUI di wxWidgets per elaborare click ed eventi
			}
		}
	}

	// Restituisce true se l'esecuzione è completata con successo, 
	// false se è stata interrotta anzitempo dall'utente (m_Running == false)
	return m_Running;
}


std::vector<sCommand> cExecutor::FetchStepsFromDatabase(uint16_t m_MasterId, uint16_t from, uint16_t to) {
	std::vector<sCommand> steps;

	// Ottimizzazione preventiva della memoria se conosciamo il range teorico massimo
	if (to >= from)	steps.reserve(static_cast<size_t>(to - from + 1));

	int64_t		detailProg = from;
	sCommand	vStep;
	bool		recordFound = false;
	cDBSampler yy(SQLLITEDBPATH);

	do {
		if (!m_Running) break;	// Se il thread viene fermato esternamente, interrompiamo subito la lettura dal DB

		recordFound = yy.ProgDetail_Select(m_MasterId, detailProg, vStep);
		if (recordFound) {
			steps.push_back(vStep);
			detailProg = vStep.m_DetailProg + 1;
		}
	} while (recordFound && m_Running && detailProg <= to);
	return steps;
}

bool cExecutor::ExecuteStep(sCommand& vStep) {
	if (!m_Running) return false;

	std::deque<sCommand> executionQueue;
	executionQueue.push_back(vStep);

	return ProcessExecutionQueue(executionQueue, nullptr, false); // Delega il lavoro al motore unificato (nessuna callback passata)
}

bool cExecutor::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	if (!m_Running) return false;

	wxStopWatch     swExecFromDB;

	auto initialSteps = FetchStepsFromDatabase(m_MasterId, from, to); // Carica i passi iniziali dal database
	if (initialSteps.empty()) return false;

	// Inizializza la coda muovendo direttamente gli elementi dal vettore
	std::deque<sCommand> executionQueue;
	for (auto& step : initialSteps) {
		executionQueue.push_back(std::move(step));
	}

	bool Ret = ProcessExecutionQueue(executionQueue, onStepCallback, false);	// Delega il lavoro al motore unificato passando la callback dell'utente
	wxString s;
	if (from == 0 && to == wxUINT16_MAX)		s = wxString::Format("Programm %ld took %ld ms.", m_MasterId, swExecFromDB.Time());
	else if (from == 0 && to != wxUINT16_MAX)	s = wxString::Format("Programm %ld (Until %ld) took %ld ms.", m_MasterId, to, swExecFromDB.Time());
	else if (from != 0 && to == wxUINT16_MAX)	s = wxString::Format("Programm %ld (from %d) took %ld ms.", m_MasterId, from, swExecFromDB.Time());
	wxString bar = wxString('*', s.Len() + 8);

	LogMe(wxString::Format("\n%s\n"
		"*** %s ***\n"
		"%s\n\n", bar, s, bar), false);

	return Ret;
}

void cExecutor::ExecuteStepSingle(sCommand& step) {
	IsRunningSet(true);
	ExecuteStep(step);
	IsRunningSet(false);
}

int cExecutor::GetMotorSelected() {
	return (m_ptrAnswerShow ? m_ptrAnswerShow->GetMotorSelected() : -1);
}

void cExecutor::PoolElements() {
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
	//		AskMotor.m_MasterId		= 0;			//2
	//		AskMotor.m_DetailProg	= 0;			//2
	//		AskMotor.m_ChkSum		= 0;			//2

	ExecuteStepSingle(AskMotor);
}


void cExecutor::OnTimer(wxTimerEvent&) {
	m_Timer.Stop();
#if !defined(USE_THREAD)

#if defined(USE_FSA_EXEC)
	fsaExecutor();
#endif
#if defined(USE_FSA_QUEUE)
	fsaEnqueue();
#endif
#endif
	if (m_CommPort.IsWorking() && m_PoolMotors && !m_Running) {
		PoolElements();
	}
	m_Timer.Start(20);	// Restart timer
}
