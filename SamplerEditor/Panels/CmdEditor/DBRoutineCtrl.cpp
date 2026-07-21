#include "stdwx.h"
#include "DBRoutineCtrl.h"

void DBRoutineCtrl::FillRoutines(void) {
	cDBSampler yy(SQLLITEDBPATH);
	wxArrayString	Names;
	wxArrayInt		Codes;
	yy.ProgMaster_FillRoutines(Names, Codes, true);
	m_RoutineList->Clear();
	size_t Cnt = Names.Count();
	for (size_t i = 0; i < Cnt; i++) {
		m_RoutineList->Append(Names[i], reinterpret_cast<void*>(static_cast<intptr_t>(Codes[i])));
	}
}
