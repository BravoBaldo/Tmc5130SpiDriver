#include "stdwx.h"
#include "CoordDBctrl.h"

#if !defined(COORDDB_IN_TEST)

void CoordDBctrl::OnChange(wxCommandEvent& Evt) {
	long DefVal=0;
	wxString Descr;
	wxString DefaultName;
	bool up = ChkThreshold();// Val > (int)m_Theshold;
	if (up) {
		int Val = m_spinPos->GetValue();
		DefaultName = wxString::Format("%s%03d", m_Prefix, Val - m_Theshold);

		cDBSampler yy(SQLLITEDBPATH);
		yy.Defaults_Get(DefaultName, DefVal, Descr);
	}
	m_spinDBVal->SetValue(DefVal);
	m_txtDescr->SetValue(Descr);
	m_txtDescr->SetHint("Insert Description");
	m_txtDBName->SetLabel(wxString::Format("%s", DefaultName));
	UpdateInterface();
	Evt.Skip();
}

void CoordDBctrl::OnUpdate(wxCommandEvent& /*Evt*/) {
	int Val = m_spinPos->GetValue();
	wxString DefaultName = wxString::Format("%s%03d", m_Prefix, Val - m_Theshold);
	cDBSampler yy(SQLLITEDBPATH);
	long DBVal = m_spinDBVal->GetValue();
	wxString Descr = m_txtDescr->GetValue();
	yy.Defaults_Set(DefaultName, DBVal, Descr);
}
#endif