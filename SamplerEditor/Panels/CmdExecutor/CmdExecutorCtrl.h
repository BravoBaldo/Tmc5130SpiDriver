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
	cDetailListCtrl*	m_ptrPrgDetail	= nullptr;

    wxTimer 	m_timer;
    cExecutor   m_Executor;

	void		OnBtnCommands	(wxCommandEvent& Evt);
	void		OnTimer			(wxTimerEvent& Evt);
	void		ShowCurrentStep	(long i, sCommand& vStep);
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
	bool		ExecuteSteps_FromTo (long from, long to);

	void		ExecuteProcess		(uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX, std::function<void(uint16_t n, sCommand&)> onStepCallback = nullptr);
    void		SetPoolMotors       (bool s, bool r = true) { m_Executor.SetPoolMotors(s, r); }
    void		IncPoolIdx          ()                      { m_Executor.IncPoolIdx(); }
    void		SetPoolIdx          (int idx)               { m_Executor.SetPoolIdx(idx); }
    int			GetMotorSelected    ()                      { return m_Executor.GetMotorSelected(); }
	bool        IsRunning			() const				{ return m_Executor.IsRunning(); }

    void		SetAnswerHandler(cAnswersShow* phandler)    { m_Executor.SetAnswerHandler(phandler); }
};