#pragma once
#include "wx/wx.h"
#include <wx/timectrl.h>
#include <wx/spinctrl.h>
#include <wx/dateevt.h>

#include "sSampler_Commands.h"

class CmdParLabel : public wxPanel {
public:
	wxStaticText	*m_Lbl_Param	= nullptr;
	wxControl		*m_gen_Param	= nullptr;
	int				m_ValidValue	= 0;
	eParType		m_type			= eChoice;
	void InitLabel		(const wxString& name);
	void SetSizers		(void);
	void ReposeSizers	(void);
	~CmdParLabel		() {};
	CmdParLabel			(wxWindow* parent, const wxString& name);									//neutral
	
	void Show			(boolean s) { m_Lbl_Param->Show(s); m_gen_Param->Show(s); };
	void SetLabel		(const wxString& T);
	void SetToolTip		(const wxString& s) { m_gen_Param->SetToolTip(s); };

	void ChangeType		(const wxString& name);											//Unknown
	void ChangeType		(const wxString& name, wxArrayString Names, wxArrayInt Codes);	//eChoice
	void ChangeType		(const wxString& name, int Min, int Max);						//eNumber
	void ChangeType		(const wxString& name, const wxDateTime& dt);					//eTime

	void SetValue(wxArrayString Names, wxArrayInt WXUNUSED(Codes));	//eChoice
	void SetValue(int Val, int Min, int Max);						//eNumber
	void SetValue(wxUint32 t);										//eTime
	void SetCurrentValue(long t);

	long GetValue(void) {
		switch (m_type) {
			case eChoice:
				{
					wxChoice* Cho = (wxChoice*)m_gen_Param;
					int Sel = Cho->GetSelection();
					if (Sel < 0)
						return -1;// Sel = 0;
					return (int)(unsigned long long)Cho->GetClientData(Sel);
				}
				break;
			case eNumber:
				return ((wxSpinCtrl*)m_gen_Param)->GetValue();
			case eTime:
				{
					int h, m, s;
					((wxTimePickerCtrl*)m_gen_Param)->GetTime(&h, &m, &s);
					return /*1000L * */ (h * 60 * 60 + m * 60 + s);
				}
			default:
				return 0;
		}
	}
};