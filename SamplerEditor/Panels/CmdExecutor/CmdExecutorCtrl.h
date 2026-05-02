#pragma once
#include "wx/wx.h"
#include "cHIDAPI.h"
#include "CmdEditorCtrl.h"
#include "DBCmdView.h"
#include "CmdExecutorCtrl.h"

typedef enum : uint8_t { eCmdOk, eCmdRetry, eCmdError }eCmdAnswer;
#pragma pack(push, 1) // Salva l'allineamento attuale e imposta a 1 byte
typedef struct _sCommAnsw {
	char		m_SubSystem;						//1
	byte		m_Cmd			= 0;				//1
	uint16_t	m_MasterId		= 0;				//2
	uint16_t	m_DetailProg	= 0;				//2

	eCmdAnswer	m_Result		= eCmdOk;			//1
	int32_t   	m_Val			= 0;				//4
	char		m_Text[30]		= "";
}sCommAnsw;
#pragma pack(pop) // Ripristina l'allineamento originale




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
	bool		ExecuteStep		(cCmdStepper& vStep);
	void		SendCommand		(const unsigned char* data, size_t length, long TimeoutMs = 500);
	eCmdAnswer	ParseAnswer(const sCommAnsw& Answ);
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
};