#pragma once
#include "wx/wx.h"
#include "cHIDAPI.h"
#include "CmdEditorCtrl.h"
#include "DBCmdView.h"

class CmdExecutorCtrl : public wxPanel {
	wxButton*	m_Btn_ExecAll = nullptr;
	wxButton*	m_Btn_ExecStep = nullptr;
	wxButton*	m_Btn_Panic = nullptr;
	wxTimer*	m_timer;
	cHIDAPI		m_HidExec;
	sVID_PID	m_HidInfo;
	bool		m_Running = false;
	//....................................
	CmdEditorCtrl*		m_ptrEditor		= nullptr;
	cDetailListCtrl*	m_ptrPrgDetail	= nullptr;
	//...................................
	DECLARE_EVENT_TABLE()
	void		OnBtnCommands	(wxCommandEvent& Evt);
	void		OnTimer			(wxTimerEvent& Evt);
	bool		ExecuteSteps	(long from, long to);
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
	bool		ExecuteFrom(long from, long to);
};