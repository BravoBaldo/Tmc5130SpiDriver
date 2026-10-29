#include "stdwx.h"
#include "cExecTools.h"

void tool_LogStep(const sCommand& vStep, bool f) {
	const sSampler_Commands* x = Command_GetByCmd(vStep.m_SubSystem, vStep.m_Cmd, vStep.m_PatLen);
	LogMe(wxString::Format(" Step %5d/%03d, cmd:%3d (%c) %-25s\t%s\t",
		vStep.m_MasterId, vStep.m_DetailProg, vStep.m_Cmd, vStep.m_Cmd, (x ? x->Descr : "UNKNOWN CMD"), (f ? "XXX" : "YYY")), true);
}

std::vector<sCommand> tool_FetchStepsFromDB(std::atomic<bool>* ptrIsRunning, uint16_t m_MasterId, uint16_t from, uint16_t to) {
	std::vector<sCommand> steps;
	if (to < from) return steps;
	steps.reserve(static_cast<size_t>(to - from + 1));

	int64_t		detailProg = from;
	sCommand	vStep;
	cDBSampler yy(SQLLITEDBPATH);

	while (detailProg <= to) {
		// Controllo immediato del thread di stop
		if (ptrIsRunning && !ptrIsRunning->load(std::memory_order_relaxed))	break;

		if (yy.ProgDetail_Select(m_MasterId, detailProg, vStep)) {
			steps.push_back(vStep);
			detailProg = vStep.m_DetailProg + 1;
		} else {
			break; // Nessun record trovato, interrompe il ciclo
		}
	}
	return steps;
}

sCommand tool_Fetch(std::atomic<bool>* ptrIsRunning, std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow) {
	if (executionQueue.empty()) {
		LogMe("ERROR: tool_Fetch chiamata su una coda di esecuzione vuota!", true);
		return sCommand{}; // Ritorna un comando vuoto di default
	}
	// Estrae l'elemento in testa usando lo spostamento (std::move) per massimizzare le prestazioni
	sCommand currentStep = std::move(executionQueue.front());
	executionQueue.pop_front();

	if (flagShow) {
		// Esegue la notifica sulla callback per l'interfaccia se presente
		if (onStepCallback) onStepCallback(currentStep);
		tool_LogStep(currentStep, false); // AAA always log!
	}

	// CASO A: Il comando estratto è una chiamata a una sotto-routine
	if (currentStep.m_SubSystem == eSystemCmd && (currentStep.m_Cmd == 'a')) {
		LogMe(wxString::Format("-> Espansione SubRoutine %d...\n", currentStep.m_Par[0]), false);
		auto subSteps = tool_FetchStepsFromDB(ptrIsRunning, currentStep.m_Par[0], 0, wxUINT16_MAX);

		// Inserisce i nuovi passi in TESTA alla coda in ordine inverso (LIFO)
		for (auto it = subSteps.rbegin(); it != subSteps.rend(); ++it) {
			executionQueue.push_front(std::move(*it));
		}
	}
	return currentStep;
}

void tool_SureCommandFromDBOld(sCommand& vStep) {
	std::unique_ptr<cDBSampler> dbPtr = nullptr;
	for (size_t i = 0; i < vStep.m_PatLen; ++i) {
		if (vStep.m_Pattern[i] == 'S' && vStep.m_Par[i] >= 50000) {
			if (!dbPtr) dbPtr = std::make_unique<cDBSampler>(SQLLITEDBPATH);
			vStep.m_Par[i] = dbPtr->Defaults_NazSteps(vStep.m_Par[i] - 50000);
		}
	}
	vStep.m_ChkSum = add_checksum_fast(reinterpret_cast<const uint8_t*>(&vStep), sizeof(vStep) - sizeof(vStep.m_ChkSum));
}

void tool_SureCommandFromDB(sCommand& vStep) {
	bool dbInitialized = false;
	cDBSampler* dbPtr = nullptr;
	alignas(cDBSampler) char dbBuffer[sizeof(cDBSampler)]; // Buffer per piazzare l'oggetto nello stack se serve
	for (size_t i = 0; i < vStep.m_PatLen; ++i) {
		if (vStep.m_Pattern[i] == 'S' && vStep.m_Par[i] >= 50000) {
			if (!dbPtr) {
				dbPtr = new (dbBuffer) cDBSampler(SQLLITEDBPATH); // Placement new sullo stack
				dbInitialized = true;
			}
			vStep.m_Par[i] = dbPtr->Defaults_NazSteps(vStep.m_Par[i] - 50000);
		}
	}

	if (dbInitialized)	dbPtr->~cDBSampler(); // Distruttore esplicito per la risorsa allocata in-place
	vStep.m_ChkSum = add_checksum_fast(reinterpret_cast<const uint8_t*>(&vStep), sizeof(vStep) - sizeof(vStep.m_ChkSum));
}