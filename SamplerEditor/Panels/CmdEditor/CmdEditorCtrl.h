#pragma once
#include "wx/wx.h"
#include <wx/vector.h>
#include <wx/timectrl.h>
#include <wx/spinctrl.h>
#include <wx/dateevt.h>

#include "sSampler_Commands.h"
#include "CmdParLabel.h"
#include "cCmdStepper.h"

//#define SHOW_PARAMS_INFO

class CmdEditorCtrl : public wxPanel {
	wxChoice		*m_cho_StepperCmd	= nullptr;

	wxTextCtrl		*m_txt_NumOfPars	= nullptr;
	wxTextCtrl		*m_txt_CmdCode		= nullptr;
	wxTextCtrl		*m_txt_ParamPattern	= nullptr;
	wxTextCtrl		*m_txt_ParamNames	= nullptr;

	wxChoice		*m_cho_Motor		= nullptr;
	CmdParLabel		*m_Params[5];

	wxTextCtrl		*m_Txt_Result		= nullptr;

//	wxStaticText	*m_Lbl_PrgMasterId;
	wxTextCtrl		*m_Txt_ProgId		= nullptr;
	wxTextCtrl		*m_Txt_StepId		= nullptr;
	long			m_ProgId = -1;
	long			m_StepId = -1;

#if defined(SOLUZIONE_ALTRA)
	wxVector<cCouple*> m_ValueList;
#endif

	void		OnChoice(wxCommandEvent& Evt);
	void		OnSpin(wxSpinEvent& Evt);
	void		OnTimeChanged(wxDateEvent& Evt);
	void		OnTextInput(wxCommandEvent& Evt);

	int			GetMotor(void);

	DECLARE_EVENT_TABLE()
protected:
public:
	~CmdEditorCtrl() {
		Params_RemoveAll();
#if defined(SOLUZIONE_ALTRA)
		for (size_t LocIdx = 0; LocIdx < m_ValueList.size(); ++LocIdx) {
			wxDELETE(m_ValueList[LocIdx]);
		}
		m_ValueList.clear();
#endif
	};
	CmdEditorCtrl(	wxWindow*		parent,
					wxWindowID		winid	= wxID_ANY,
					const wxPoint&	pos		= wxDefaultPosition,
					const wxSize&	size	= wxDefaultSize,
					long			style	= wxTAB_TRAVERSAL | wxNO_BORDER,
					const wxString& name	= wxPanelNameStr
				);


	wxString	UI2String		(void);	//From UI to Command String
	cCmdStepper	UI2DBData		(void);	//From UI to Database
	void		DBData2UI		(cCmdStepper& vStep);
	wxString	DBData2String	(cCmdStepper& vStep);
	bool		Cmd2UI			(uint8_t Mot, char Cmd, const std::vector<long>& ParValues);
	bool		String2UI		(wxString Cmd);

	bool		PoseCommand		(char c, uint8_t NumPar);

	void		SetDbInfo(long MasterId, long DetId) {
		m_ProgId = MasterId; if (m_Txt_ProgId)	m_Txt_ProgId->SetValue(wxString::Format("%ld", m_ProgId));
		m_StepId = DetId;	 if (m_Txt_StepId)	m_Txt_StepId->SetValue(wxString::Format("%ld", m_StepId));
	}
	long		GetProgId(void) { return m_ProgId; };
	long		GetStepId(void) { return m_StepId; };
};
