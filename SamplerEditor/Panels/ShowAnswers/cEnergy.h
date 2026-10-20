#pragma once
#include <wx/wx.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>

class cEnergy : public wxPanel {
	private:
		wxStaticText*	m_lbl_Volt;
		wxTextCtrl*		m_txt_Volt;
		wxGauge*		m_gau_Volt;
		wxStaticText*	m_lbl_Amp;
		wxTextCtrl*		m_txt_Amp;
		wxGauge*		m_gau_Amp;
		wxStaticText*	m_lbl_Watt;
		wxTextCtrl*		m_txt_Watt;
		wxGauge*		m_gau_Watt;
public:
	cEnergy(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString);
	~cEnergy() {};
	void Fill(const AnswerHeader* ptrHeader);
};
