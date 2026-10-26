
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

cExecutor::cExecutor() : m_CommPort(), m_Timer(this) {
	this->Bind(wxEVT_TIMER, &cExecutor::OnTimer, this, m_Timer.GetId());
	m_Timer.Start(100);	// millisecond interval
	m_ResponseBuffer.reserve(sizeof(AnswerHeader)+2);
	m_Running = false;
#if defined(USE_FSA_EXEC)
	m_State = STATE_IDLE;
#endif
}

cExecutor::~cExecutor() {
	m_Running = false;
	m_Timer.Stop();
}

eCmdAnswer cExecutor::GetAndShowMessage(uint8_t chk, long TimeoutMs) {
	m_ResponseBuffer.clear(); 
	size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, chk, TimeoutMs);	//Aspetta una risposta al messaggio chk
	if (bytesRead <= 0)						return eErrTimeout;
	if (bytesRead < sizeof(AnswerHeader))	return eErrShortMsg;
	const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
	if (!ptrHeader)							return eErrTimeout;
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

	// Add CheckSum
	vStep.m_ChkSum = add_checksum_fast( reinterpret_cast<const uint8_t*>(&vStep), sizeof(vStep) - sizeof(vStep.m_ChkSum) );
}

#if defined(USE_FSA_EXEC)
wxString cExecutor::StateName(eExecutorState s) {
	switch (s) {
		case STATE_IDLE:				return "STATE_IDLE";				// In attesa di un nuovo ciclo o comando
		case STATE_FETCH_NEXT_COMMAND:	return "STATE_FETCH_NEXT_COMMAND";	// Estrazione del prossimo comando dalla coda
		case STATE_SEND_COMMAND:		return "STATE_SEND_COMMAND";		// Invio del comando sulla porta seriale/CAN
		case STATE_WAIT_RESPONSE:		return "STATE_WAIT_RESPONSE";		// In attesa della risposta dall'hardware (con timeout)
		case STATE_ASK_USER_ON_ERROR:	return "STATE_ASK_USER_ON_ERROR";	// Dialogo aperto con l'utente in caso di errore hardware
		case STATE_ERROR_ABORT:			return "STATE_ERROR_ABORT";			// Blocco per errore critico
	}
	return "???";
}
#else

void cExecutor::SendCommand(const sCommand& vStep, long TimeoutMs) {
	bool		Success		= false;
	int			retryCount	= 0;
	wxStopWatch	swTotCmd;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	swTotCmd.Start(0);

	while (!Success && m_Running) {
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
						default:		LogMe("\tEND PROCESS\n", false);	m_Running = false; return;
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
}

bool cExecutor::DoExecuteSingleStep(sCommand& vStep) {
	if (!m_Running) return false;
	SureCommandFromDB(vStep);
	SendCommand(vStep);	// Invio fisico del comando all'hardware
	return true;
}

bool cExecutor::ProcessExecutionQueue(std::deque<sCommand>& executionQueue, std::function<void(uint16_t, sCommand&)> onStepCallback, bool flagShow) {
	uint16_t		n = 0;
	size_t			safetyLoopCounter = 0;
	const size_t	MAX_SAFETY_STEPS = 50000;

	while (!executionQueue.empty() && m_Running) {
		if (++safetyLoopCounter > MAX_SAFETY_STEPS) {
			LogMe("\nCRITICAL ERROR: Overflow nelle subroutine (possibile loop infinito nel DB)!\n", true);
			return false;
		}

		// Ottimizzazione: Usiamo std::move per evitare copie inutili di sCommand
		sCommand currentStep = std::move(executionQueue.front());
		executionQueue.pop_front();

		if (flagShow) {
			// Esegue la notifica sulla callback per l'interfaccia se presente
			if (onStepCallback)	onStepCallback(n++, currentStep);
			LogStep(currentStep);
		}

		// CASO A: Il comando estratto è una chiamata a una sotto-routine
		if (currentStep.m_SubSystem == eSystemCmd && (currentStep.m_Cmd == 'a')) {
			LogMe(wxString::Format("-> Espansione Sotto-Routine %d...\n", currentStep.m_Par[0]), false);
			auto subSteps = FetchStepsFromDatabase(currentStep.m_Par[0], 0, wxUINT16_MAX);	// Estrae i passi della sotto-routine dal DB

			// Inserisce i nuovi passi in TESTA alla coda in ordine inverso (LIFO)
			for (auto it = subSteps.rbegin(); it != subSteps.rend(); ++it) {
				executionQueue.push_front(std::move(*it));
			}
			continue;
		}
		DoExecuteSingleStep(currentStep);	// CASO B: È un comando hardware standard. Lo eseguiamo atomicamente.

		::wxYield();	// Cede il controllo al sistema operativo per non congelare l'interfaccia grafica wxWidgets
	}

	return true;
}

#endif

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
#if defined(USE_FSA_EXEC)
	if (m_State != STATE_IDLE) return false; // Macchina già occupata
	m_ExecutionQueue.clear();
	m_ExecutionQueue.push_back(vStep);
	m_State = STATE_FETCH_NEXT_COMMAND;
	return true;
#else
	if (!m_Running) return false;

	std::deque<sCommand> executionQueue;
	executionQueue.push_back(vStep);

	return ProcessExecutionQueue(executionQueue, nullptr, false); // Delega il lavoro al motore unificato (nessuna callback passata)
#endif
}

bool cExecutor::ExecuteSteps_FromDB(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(uint16_t, sCommand&)> onStepCallback) {
#if defined(USE_FSA_EXEC)
	if (m_State != STATE_IDLE) return false;

	auto initialSteps = FetchStepsFromDatabase(m_MasterId, from, to);
	if (initialSteps.empty()) return false;

	m_ExecutionQueue.clear();
	for (auto& step : initialSteps) {
		m_ExecutionQueue.push_back(std::move(step));
	}
	m_State = STATE_FETCH_NEXT_COMMAND;
	return true;
#else
	if (!m_Running) return false;

	auto initialSteps = FetchStepsFromDatabase(m_MasterId, from, to); // Carica i passi iniziali dal database
	if (initialSteps.empty()) return false;

	// Inizializza la coda muovendo direttamente gli elementi dal vettore
	std::deque<sCommand> executionQueue;
	for (auto& step : initialSteps) {
		executionQueue.push_back(std::move(step));
	}

	return ProcessExecutionQueue(executionQueue, onStepCallback, false);	// Delega il lavoro al motore unificato passando la callback dell'utente
#endif
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
#if defined(USE_FSA_EXEC)
	if (m_Running) {
		ProcessStateMachine();	// Esegue un passo della macchina a stati

		// Se la macchina è in IDLE, esegue il vecchio polling dei motori (se necessario)
		if (m_State == STATE_IDLE && m_PoolMotors) {
			PoolElements();
		}
	}
#else
	if (m_CommPort.IsWorking() && m_PoolMotors && !m_Running) {
		PoolElements();
	}
#endif

	m_Timer.Start(20);	// Restart timer
}

#if defined(USE_FSA_EXEC)
void cExecutor::ProcessStateMachine() {
	if (m_State == STATE_IDLE) return;

	switch (m_State) {
		case STATE_IDLE:
			break;			// Non fa nulla, in attesa che un metodo pubblico carichi la coda

		case STATE_FETCH_NEXT_COMMAND:
			if (m_ExecutionQueue.empty()) {
				LogMe("Ciclo di esecuzione completato con successo.\n", false);
				m_State = STATE_IDLE;
				break;
			}

			// Estrae il comando dalla testa (Asincrono e senza copie)
			m_CurrentStep = std::move(m_ExecutionQueue.front());
			m_ExecutionQueue.pop_front();
			m_RetryCount = 0;

			// Controllo ed espansione Subroutine ('a')
			if (m_CurrentStep.m_SubSystem == eSystemCmd && (m_CurrentStep.m_Cmd == 'a')) {
				LogMe(wxString::Format("-> Espansione Sotto-Routine %d...\n", m_CurrentStep.m_Par[0]), false);
				auto subSteps = FetchStepsFromDatabase(m_CurrentStep.m_Par[0], 0, wxUINT16_MAX);

				// Inserimento LIFO in testa alla coda
				for (auto it = subSteps.rbegin(); it != subSteps.rend(); ++it) {
					m_ExecutionQueue.push_front(std::move(*it));
				}
				m_State = STATE_FETCH_NEXT_COMMAND; // Salta subito a prendere il primo sottostep
				break;
			}

			SureCommandFromDB(m_CurrentStep); // Risoluzione costanti 'S' da database (Logica atomica ex DoExecuteSingleStep)

			m_State = STATE_SEND_COMMAND;
			break;

		case STATE_SEND_COMMAND:	// Scrittura asincrona non bloccante sulla porta
			if (!m_CommPort.Write(&m_CurrentStep, sizeof(sCommand), 100)) {
				LogMe("Hardware error writing port. Aborting.\n", true);
				m_State = STATE_ERROR_ABORT;
				break;
			}

			// Configura il timeout di attesa risposta e avvia il cronometro di stato
			m_CurrentTimeoutMs = (m_CurrentStep.m_DetailProg > 0) ? m_CurrentStep.m_DetailProg : 500;
			m_StateWatch.Start();
			m_State = STATE_WAIT_RESPONSE;
			break;

		case STATE_WAIT_RESPONSE:
			{
				m_ResponseBuffer.clear();
				size_t bytesRead = m_CommPort.Read(m_ResponseBuffer, m_CurrentStep.m_Cmd, 0); // Timeout = 0 (Lettura immediata non bloccante)

				if (bytesRead >= sizeof(AnswerHeader)) {
					const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(m_ResponseBuffer.data());
					if (m_ptrAnswerShow) m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);

					// Valutazione risposta hardware
					if (ptrHeader->m_Result == eCmdOk) {
						m_State = STATE_FETCH_NEXT_COMMAND; // Successo! Passa al prossimo comando
					} else if (ptrHeader->m_Result == eCmdRetry) {
						m_State = STATE_SEND_COMMAND;       // Richiesta di riprovare dall'hardware
					} else if (ptrHeader->m_Result == eCmdError) {
						m_State = STATE_ASK_USER_ON_ERROR;   // Errore logico, chiede istruzioni alla UI
					}
					break;
				}

				// Verifica se è scaduto il tempo massimo di attesa (Timeout software)
				if (m_StateWatch.Time() > m_CurrentTimeoutMs) {
					m_RetryCount++;
					LogMe(wxString::Format("Timeout (%ld ms). Retransmit (%d/50)...\n", m_CurrentTimeoutMs, m_RetryCount), true);

					if (m_RetryCount > 50) {
						LogMe("Too many attempts. Process aborted.\n", true);
						m_State = STATE_ERROR_ABORT;
					} else {
						m_State = STATE_SEND_COMMAND; // Riprova la trasmissione
					}
				}
			}
			break;

		case STATE_ASK_USER_ON_ERROR:
			{	// Mostra la finestra di dialogo. NOTA: ShowModal blocca solo la UI ma è sicuro
				wxMessageDialog dialog(nullptr, wxT("ERROR received from hardware"), wxT("Continue?"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);
				if (dialog.ShowModal() == wxID_YES) {
					LogMe("\tCONTINUE BY USER\n", false);
					m_State = STATE_FETCH_NEXT_COMMAND;
				} else {
					LogMe("\tABORTED BY USER\n", false);
					m_State = STATE_ERROR_ABORT;
				}
			}
			break;

		case STATE_ERROR_ABORT:
			m_ExecutionQueue.clear();
			m_Running = false;
			m_State = STATE_IDLE;
			LogMe("Macchina a stati fermata per errore critico.\n", true);
			break;
	}
}
#endif
