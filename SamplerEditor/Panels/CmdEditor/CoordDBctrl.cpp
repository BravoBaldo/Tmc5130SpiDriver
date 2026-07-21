#include "stdwx.h"
#include "CoordDBctrl.h"


void CoordDBctrl::OnChange(wxCommandEvent& ) {
	int Val = m_spinPos->GetValue();
	long DefVal=0;
	wxString Descr;
	wxString DefaultName;
	bool up = Val > (int)m_Theshold;
	if (up) {
		DefaultName = wxString::Format("%s%03d", m_Prefix, Val - m_Theshold);

		cDBSampler yy(SQLLITEDBPATH);
		yy.Defaults_Get(DefaultName, DefVal, Descr);
	}
	m_spinDBVal->SetValue(DefVal);
	m_txtDescr->SetValue(Descr);
	m_txtDBName->SetLabel(wxString::Format("%s", DefaultName));
	ShowExtraControls(up);
}

void CoordDBctrl::OnUpdate(wxCommandEvent& /*event*/) {
	int Val = m_spinPos->GetValue();
	wxString DefaultName = wxString::Format("%s%03d", m_Prefix, Val - m_Theshold);
	cDBSampler yy(SQLLITEDBPATH);
	long DBVal = m_spinDBVal->GetValue();
	wxString Descr = m_txtDescr->GetValue();
	yy.Defaults_Set(DefaultName, DBVal, Descr);
}
