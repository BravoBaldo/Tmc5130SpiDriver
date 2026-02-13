#include "stdwx.h"
#include "CmdParLabel.h"

void CmdParLabel::SetLabel(const wxString& T) {
	m_Lbl_Param->SetLabel(T);
}

void CmdParLabel::InitLabel(const wxString& name) {
	wxDELETE(m_Lbl_Param);	//Also remove it from Sizer
	m_Lbl_Param = new wxStaticText(this, wxID_ANY, name);
}

void CmdParLabel::ChangeType(const wxString& name) {		//Unknown
	m_type = eUnknown;	InitLabel(name);

	wxDELETE(m_gen_Param);
	m_gen_Param = new wxStaticText(this, wxID_ANY, "unknown");
	SetSizers();
}

void CmdParLabel::SetCurrentValue(long t) {
	switch (m_type) {
		case eChoice:	((wxChoice*)m_gen_Param)->SetSelection(t);	break;
		case eNumber:	((wxSpinCtrl*)m_gen_Param)->SetValue(t);	break;
		case eTime:
			{
				unsigned int s = t % 60;		t /= 60;
				unsigned int m = t % 60;		t /= 60;
				unsigned int h = t % 24;	//	t/=24;
				((wxTimePickerCtrl*)m_gen_Param)->SetTime(h, m, s);
			}
			break;
		default:
			return;
	}
}

void CmdParLabel::SetValue(wxArrayString Names, wxArrayInt WXUNUSED(Codes)) {	//eChoice
	wxChoice* Cho = (wxChoice*)m_gen_Param;
	Cho->Clear();
	size_t Cnt = Names.Count();
	for (size_t i = 0; i < Cnt; i++) {
		Cho->Append(Names[i], (void*)i/*Codes[i]*/);
	}
	Cho->SetSelection(0);
}

void CmdParLabel::SetValue(int Val, int Min, int Max) {	//eNumber
	((wxSpinCtrl*)m_gen_Param)->SetRange(Min, Max);
	((wxSpinCtrl*)m_gen_Param)->SetValue(Val);
}

void CmdParLabel::SetValue(wxUint32 t) {
	unsigned int s = t % 60;		t /= 60;
	unsigned int m = t % 60;		t /= 60;
	unsigned int h = t % 24;	//	t/=24;
	((wxTimePickerCtrl*)m_gen_Param)->SetTime(h, m, s);
}

void CmdParLabel::ChangeType(const wxString& name, const wxDateTime& dt) {	//eTime
	m_type = eTime;	InitLabel(name);

	wxDELETE(m_gen_Param);
	m_gen_Param = new wxTimePickerCtrl(this, wxID_ANY, dt);
	SetSizers();
}

CmdParLabel::CmdParLabel(wxWindow* parent, const wxString& name) : wxPanel(parent, wxID_ANY) {	//neutral
	//SetBackgroundColour(wxColour(88, 100, 155));
	m_Lbl_Param = new wxStaticText(this, wxID_ANY, name);
	m_gen_Param = new wxStaticText(this, wxID_ANY, name);
}

void CmdParLabel::ChangeType(const wxString& name, wxArrayString Names, wxArrayInt WXUNUSED(Codes)) {	//eChoice
	m_type = eChoice;	InitLabel(name);
	wxDELETE(m_gen_Param);
	m_gen_Param = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL/*, wxCB_SIMPLE/*| wxCB_SORT*/);
	size_t Cnt = Names.Count();
	for (size_t i = 0; i < Cnt; i++) {
		((wxChoice*)m_gen_Param)->Append(Names[i], (void*)i/*Codes[i]*/);
	}
	((wxChoice*)m_gen_Param)->SetSelection(0);
	SetSizers();
}

void CmdParLabel::ChangeType(const wxString& name, int Min, int Max) {	//eNumber
	m_type = eNumber;	InitLabel(name);

	wxDELETE(m_gen_Param);
	m_gen_Param = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, Min, Max, Min);
	SetSizers();
}

void CmdParLabel::ReposeSizers(void) {
	wxSizer* sizMaster = GetSizer(); // use the sizer for layout
	sizMaster->Fit(this); // fit the dialog to the contents
	sizMaster->SetSizeHints(this); // set hints to honor min size
}

void CmdParLabel::SetSizers(void) {
	//SIZER_STATDEBUG(sizMaster, "par", wxHORIZONTAL);
	wxBoxSizer* sizMaster = new wxBoxSizer(wxHORIZONTAL);
	sizMaster->Add(m_Lbl_Param, 1, wxALL, 0);
	sizMaster->Add(m_gen_Param, 1, wxALL, 0);

	SetSizer(sizMaster); // use the sizer for layout
	ReposeSizers();
}
