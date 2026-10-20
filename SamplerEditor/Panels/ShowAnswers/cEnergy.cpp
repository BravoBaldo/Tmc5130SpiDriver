#include "stdwx.h"
#include "cEnergy.h"

cEnergy::cEnergy(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxPanel(parent, id, pos, size, style, name)
{
	m_lbl_Volt = new wxStaticText(this, wxID_ANY, _("mV:"));//					m_lbl_Volt->Wrap(-1);
	m_txt_Volt = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	m_gau_Volt = new wxGauge(this, wxID_ANY, 25000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);	m_gau_Volt->SetValue(55);

	m_lbl_Amp = new wxStaticText(this, wxID_ANY, _("mA:"));//					m_lbl_Amp->Wrap(-1);
	m_txt_Amp = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	m_gau_Amp = new wxGauge(this, wxID_ANY, 2000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);	m_gau_Amp->SetValue(15);

	m_lbl_Watt = new wxStaticText(this, wxID_ANY, _("mW:"));//					m_lbl_Watt->Wrap(-1);
	m_txt_Watt = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
	m_gau_Watt = new wxGauge(this, wxID_ANY, 50000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);	m_gau_Watt->SetValue(77);


	auto* boxContainer = new wxStaticBox(this, wxID_ANY, _("Energy"));
	auto* sizMaster = new wxStaticBoxSizer(boxContainer, wxVERTICAL);

	auto* sizPowers = new wxFlexGridSizer(3, 3, 0, 0);
	sizPowers->AddGrowableCol(2);
	sizPowers->SetFlexibleDirection(wxHORIZONTAL); // Cambiato da BOTH a HORIZONTAL per bloccare l'altezza
	sizPowers->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_NONE); // Evita espansioni verticali delle celle

#define ADD_ROW(lbl, txt, gau) \
	sizPowers->Add(lbl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4); \
	sizPowers->Add(txt, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4); \
	sizPowers->Add(gau, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 4);

	ADD_ROW(m_lbl_Volt,	m_txt_Volt,	m_gau_Volt)
	ADD_ROW(m_lbl_Amp,	m_txt_Amp,	m_gau_Amp)
	ADD_ROW(m_lbl_Watt,	m_txt_Watt,	m_gau_Watt)
#undef ADD_ROW

	sizMaster->Add(sizPowers, 0, wxEXPAND | wxALL, 2);

	this->SetSizer(sizMaster);
	sizMaster->Fit(this); // Forza il pannello a rimpicciolirsi attorno al suo sizer
	this->Layout();
}

void cEnergy::Fill(const AnswerHeader* ptrHeader) {
	float V = 0.0f, A = 0.0f, W = 0.0f;
#if defined(USE_INA260)	
	switch (ptrHeader->m_MsgType) {
		case eTypAnswStepDir:	//TmcAnswer
			{
				TmcAnswer Answ = *reinterpret_cast<const TmcAnswer*>(ptrHeader);
				V = Answ.m_Volt;
				A = Answ.m_Curr;
				W = Answ.m_Power;
			}
			break;
		case eTypAnswFsaSingle:	//FSA Single Stepper
			{
				FsaSingleAnswer Answ = *reinterpret_cast<const FsaSingleAnswer*>(ptrHeader);
				V = Answ.m_Volt;
				A = Answ.m_Curr;
				W = Answ.m_Power;
			}
			break;
		default:
				return;
	}
#endif
	m_txt_Volt	->SetValue(wxString::Format("%06.2f", V));	m_gau_Volt	->SetValue(V);
	m_txt_Amp	->SetValue(wxString::Format("%06.2f", A));	m_gau_Amp	->SetValue(A);
	m_txt_Watt	->SetValue(wxString::Format("%07.0f", W));	m_gau_Watt	->SetValue(W);
}
