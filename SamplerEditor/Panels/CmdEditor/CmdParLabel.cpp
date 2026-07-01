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
	if (!m_gen_Param) return;
	switch (m_type) {
		case eChoice: {
			if (auto* cho = wxDynamicCast(m_gen_Param, wxChoice)) {
				if (t >= 0 && t < static_cast<long>(cho->GetCount())) {
					cho->SetSelection(static_cast<int>(t));
				} else if (t == -1) {
					cho->SetSelection(wxNOT_FOUND);
				}
			}
			break;
		}

		case eNumber: {
			if (auto* spinCtrl = wxDynamicCast(m_gen_Param, wxSpinCtrl)) {
				int minVal = spinCtrl->GetMin();
				int maxVal = spinCtrl->GetMax();
				long safeVal = std::max(static_cast<long>(minVal), std::min(t, static_cast<long>(maxVal)));
				spinCtrl->SetValue(static_cast<int>(safeVal));
			}
			break;
		}

		case eTime: {
#if defined(USE_DATE_TIME_CTRL)
			if (auto* timePicker = wxDynamicCast(m_gen_Param, wxTimePickerCtrl)) {
				unsigned long absTime = (t < 0) ? 0 : static_cast<unsigned long>(t);

				unsigned int s = absTime % 60;          absTime /= 60;
				unsigned int m = absTime % 60;          absTime /= 60;
				unsigned int h = absTime % 24;

				timePicker->SetTime(h, m, s);
			}
#else
			if (auto* spinCtrl = wxDynamicCast(m_gen_Param, wxSpinCtrl)) {
				int minVal = 0;
				int maxVal = MAXSECS;
				long safeVal = std::max(static_cast<long>(minVal), std::min(t, static_cast<long>(maxVal)));
				spinCtrl->SetValue(static_cast<int>(safeVal));
			}
#endif
			break;
		}

		default:
			return;
	}
}

long CmdParLabel::GetValue(void) {
	if (!m_gen_Param) return 0;
	switch (m_type) {
		case eChoice:
			{
				if (auto* cho = wxDynamicCast(m_gen_Param, wxChoice)) {
					int sel = cho->GetSelection();
					if (sel == wxNOT_FOUND)		return -1;
					wxUIntPtr data = reinterpret_cast<wxUIntPtr>(cho->GetClientData(sel));
					return static_cast<long>(data);
				}
			}
			break;
		case eNumber:
			{
				if (auto* spinCtrl = wxDynamicCast(m_gen_Param, wxSpinCtrl))
					return static_cast<long>(spinCtrl->GetValue());
			}
			break;
		case eTime:
#if defined(USE_DATE_TIME_CTRL)
			if (auto* timePicker = wxDynamicCast(m_gen_Param, wxTimePickerCtrl)) {
				int h = 0, m = 0, s = 0;
				timePicker->GetTime(&h, &m, &s);
				long secondiTotali = static_cast<long>(h) * 3600 + static_cast<long>(m) * 60 + s;
				return secondiTotali;
			}
#else
			if (auto* spinCtrl = wxDynamicCast(m_gen_Param, wxSpinCtrl))
				return static_cast<long>(spinCtrl->GetValue());
#endif
			break;
		default:
			break;
	}
	return 0;
}


void CmdParLabel::SetValue(wxArrayString Names, wxArrayInt WXUNUSED(Codes)) {	//eChoice
	wxChoice* Cho = wxDynamicCast(m_gen_Param, wxChoice);
	if (!Cho) return;
	Cho->Clear();
	size_t Cnt = Names.Count();
	for (size_t i = 0; i < Cnt; i++) {
		//Cho->Append(Names[i], (void*)i/*Codes[i]*/);
		//wxUIntPtr dataValue = useCodes ? static_cast<wxUIntPtr>(Codes[i]) : static_cast<wxUIntPtr>(i);
		Cho->Append(Names[i], reinterpret_cast<void*>(i));


	}
	if (Cnt > 0)	Cho->SetSelection(0);
}

void CmdParLabel::SetValue(int Val, int Min, int Max) {	//eNumber
	wxSpinCtrl* spinCtrl = wxDynamicCast(m_gen_Param, wxSpinCtrl);
	if (!spinCtrl) return;
	spinCtrl->SetRange(Min, Max);
	int safeVal = std::max(Min, std::min(Val, Max));
	spinCtrl->SetValue(safeVal);
}

void CmdParLabel::SetValue(wxUint32 t) {
#if defined(USE_DATE_TIME_CTRL)
	wxTimePickerCtrl* timePicker = wxDynamicCast(m_gen_Param, wxTimePickerCtrl);
	if (!timePicker) return;
	wxUint32 timeRemain = t;
	unsigned int s = timeRemain % 60;          timeRemain /= 60;
	unsigned int m = timeRemain % 60;          timeRemain /= 60;
	unsigned int h = timeRemain % 24;
	timePicker->SetTime(h, m, s);
#else
	SetValue(t, 0, MAXSECS);
#endif
}

void CmdParLabel::ChangeType(const wxString& name, const wxDateTime& dt) {	//eTime
	m_type = eTime;	InitLabel(name);

	if (m_gen_Param) {	// do not use wxDELETE(m_gen_Param);
		m_gen_Param->Destroy();
		m_gen_Param = nullptr;
	}
#if defined(USE_DATE_TIME_CTRL)
	m_gen_Param = new wxTimePickerCtrl(this, wxID_ANY, dt);
#else
	SetValue(dt.GetMinute() * 60 + dt.GetSecond(), 0, MAXSECS);
#endif
	SetSizers();
}

CmdParLabel::CmdParLabel(wxWindow* parent, const wxString& name) : wxPanel(parent, wxID_ANY) {	//neutral
	//SetBackgroundColour(wxColour(88, 100, 155));
	m_Lbl_Param = new wxStaticText(this, wxID_ANY, name);
	m_gen_Param = new wxStaticText(this, wxID_ANY, name);
}

void CmdParLabel::ChangeType(const wxString& name, wxArrayString Names, wxArrayInt WXUNUSED(Codes)) {	//eChoice
	m_type = eChoice;	InitLabel(name);
	
	if (m_gen_Param) {	//Do not use wxDELETE(m_gen_Param);
		m_gen_Param->Destroy();
		m_gen_Param = nullptr;
	}
	wxChoice* cho = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL/*, wxCB_SIMPLE/*| wxCB_SORT*/);
	m_gen_Param = cho;
	size_t cnt = Names.Count();
	for (size_t i = 0; i < cnt; i++) {
		wxUIntPtr dataValue = static_cast<wxUIntPtr>(i);
		cho->Append(Names[i], reinterpret_cast<void*>(dataValue));
	}
	if (cnt > 0)	cho->SetSelection(0);
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
