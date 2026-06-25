#pragma once
#include "wx/wx.h"

#include "cHIDAPI.h"
#include "CmdEditorCtrl.h"
#include "DBCmdView.h"
#include "CmdExecutorCtrl.h"
#include "cShowAnswers.h"



class CmdExecutorCtrl : public wxPanel {
	wxButton*	m_Btn_ExecAll	= nullptr;
	wxButton*	m_Btn_ExecStep	= nullptr;
	wxButton*	m_Btn_Panic		= nullptr;
	wxTimer*	m_timer			= nullptr;
	cHIDAPI		m_HidExec;
	sVID_PID	m_HidInfo;
	bool		m_Running		= false;

	ParamType	m_PoolIdx		= 0;
	bool		m_PoolMotors	= false;
	bool		m_RotatePool	= true;
	//....................................
	CmdEditorCtrl*		m_ptrEditor		= nullptr;
	cDetailListCtrl*	m_ptrPrgDetail	= nullptr;
	cAnswersShow*		m_ptrAnswerShow	= nullptr;
	//...................................
	DECLARE_EVENT_TABLE()
	void		OnBtnCommands	(wxCommandEvent& Evt);
	void		OnTimer			(wxTimerEvent& Evt);
	bool		ExecuteStep		(sCommand& vStep);
	//void		TxMessage(const unsigned char* data, size_t length, long TimeoutMs = 500);
	//void		RxMessage(const unsigned char* data, size_t length, long TimeoutMs = 500);

	void		SendCommand		(const unsigned char* data, size_t length, long TimeoutMs = 500);

	eCmdAnswer	ParseAnswer(const sAnswerStandard&		Answ);
	eCmdAnswer	ParseAnswer(const sExpanderStandard&	Answ);
	eCmdAnswer	ParseAnswer(const StripAnswer& Answ);
	eCmdAnswer	ParseAnswer(const TmcAnswer& Answ);
	eCmdAnswer	ParseAnswer(const sAnswerVersion& Answ);


public:
	CmdExecutorCtrl	(	wxWindow*		parent,
						wxWindowID		winid	= wxID_ANY,
						const wxPoint&	pos		= wxDefaultPosition,
						const wxSize&	size	= wxDefaultSize,
						long			style	= wxTAB_TRAVERSAL | wxNO_BORDER,
						const wxString&	name	= wxPanelNameStr
					);
	~CmdExecutorCtrl();
	void	SetEditorAndDB(CmdEditorCtrl* ptrEditor, cDetailListCtrl* ptrPrgDetail) {
		m_ptrEditor = ptrEditor;
		m_ptrPrgDetail = ptrPrgDetail;
	}
	bool		ExecuteSteps(long from, long to);
	bool		ExecuteSteps(uint16_t	m_MasterId);
	void		SetAnswerHandler(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
	void		SetPoolMotors	(bool s, bool r = true) { m_PoolMotors = s; m_RotatePool = r; }
	void		IncPoolIdx		(void) { 
		m_PoolIdx = (m_PoolIdx + 1) % 3; 
	}
		
};