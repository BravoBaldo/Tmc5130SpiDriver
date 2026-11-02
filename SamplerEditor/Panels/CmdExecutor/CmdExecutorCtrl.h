#pragma once
#include "wx/wx.h"

#include "cCommPort.h"
#include "cExecutor.h"

#include "CmdEditorCtrl.h"
#include "DBCmdView.h"		//cDetailListCtrl

class CmdExecutorCtrl : public wxPanel {
    // Componenti GUI gestiti dal ciclo di vita nativo di wxWidgets
	wxButton*	m_Btn_ExecAll		= nullptr;
	wxButton*	m_Btn_ExecEditor	= nullptr;
	wxButton*	m_Btn_Panic			= nullptr;

    // Puntatori a classi esterne (risolti tramite forward declaration)
	CmdEditorCtrl*		m_ptrEditor		= nullptr;
	cDetailListCtrl*	m_ptrPrgDetail	= nullptr;	//cDetailListCtrl

    wxTimer 	m_timer;
    cExecutor   m_Executor;

	void		OnBtnCommands		(wxCommandEvent& Evt);
	void		OnTimer				(wxTimerEvent& Evt);
	void		ShowCurrentStep		(long i, sCommand& vStep);
public:
	CmdExecutorCtrl	(	wxWindow*		parent,
						wxWindowID		winid	= wxID_ANY,
						const wxPoint&	pos		= wxDefaultPosition,
						const wxSize&	size	= wxDefaultSize,
						long			style	= wxTAB_TRAVERSAL | wxNO_BORDER,
						const wxString&	name	= wxPanelNameStr
					);
	~CmdExecutorCtrl();
	void	    SetEditorAndDB      (CmdEditorCtrl* ptrEditor, cDetailListCtrl* ptrPrgDetail);

	void		ExecuteProcess		(uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);
    void		SetPoolMotors       (bool enable, bool rotate) { m_Executor.SetPoolMotors(enable, rotate); }
    void		SetPoolIdx          (int idx)               { m_Executor.SetPoolIdx(idx); }
	bool        IsRunning			() const				{ return m_Executor.IsRunning(); }

	void        OnThreadNotification(wxCommandEvent& event);
	void        OnThreadAskUser(wxCommandEvent& event);

	cAnswersShow* m_ptrAnswerShow = nullptr;
	void		SetAnswerHandler(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
};