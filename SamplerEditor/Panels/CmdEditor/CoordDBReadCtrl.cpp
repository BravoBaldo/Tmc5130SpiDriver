#include "stdwx.h"
#include "CoordDBReadCtrl.h"
#include "cDBSampler.h"

#if !defined(DBROUTINEREAD_IN_TEST)
void CoordDBReadctrl::FillRoutines(bool SortByNum) {
	cDBSampler yy(SQLLITEDBPATH);
	wxArrayString	Names;
	wxArrayInt		Codes;
	yy.Defaults_FillCoords(Names, Codes, SortByNum);
	m_RoutineList->Clear();
	size_t Cnt = Names.Count();
	for (size_t i = 0; i < Cnt; i++) {
		m_RoutineList->Append(Names[i], reinterpret_cast<void*>(static_cast<intptr_t>(Codes[i])));
	}
}
#endif