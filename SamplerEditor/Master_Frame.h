//Master_Frame.h
#pragma once

#include "wx/wx.h"
#include "DBCmdView.h"
#include "CmdEditorCtrl.h"
#include "cShowAnswers.h"
//===============================================

//===============================================





#define USE_AUI
#ifdef USE_AUI
	#include "wx/aui/aui.h"
	#include "wx/notebook.h"
	#include "wx/clipbrd.h"
	static const wxString	Perspect_Key		( "MyKey" );
	static const wxString	Perspect_FileName	( "Perspectives.txt" );
	static wxConfig			Perspect_Config		( PRG_UNIQUE_FIRM );	//HKEY_CURRENT_USER\Software\Aries
#endif
//*****************************************

class MyFrame: public wxFrame {
public:
    MyFrame		(	const wxString&	title,
					const wxPoint&	pos,
					const wxSize&	size,
						  wxLocale*	locale

				);
    ~MyFrame();

private:
	void OnMouseEvent	( wxMouseEvent&		Evt );
	void OnTimer		( wxTimerEvent&		Evt );
	void OnPaint		( wxPaintEvent&		Evt );
	void OnListEvent	(wxListEvent& Evt);
	void OnClearLog		(wxMouseEvent& Evt);

	void OnMenu			( wxCommandEvent&	Evt );
	void		OnBtnCommands	( wxCommandEvent&	Evt );
	void		SetLayouts ( void );

#ifdef USE_AUI
	wxString	Perspective_Get	( void )			{ return m_mgr.SavePerspective(); }
	void		Perspective_Set	(const wxString& p)	{	if(p.Len()>0){
															this->Freeze();
															m_mgr.LoadPerspective(p);
															this->Thaw();
														}
													}
	void		AuiRefresh		( void )			{ this->Freeze(); m_mgr.Update(); this->Thaw(); }
	wxAuiManager		m_mgr;
#endif

    DECLARE_EVENT_TABLE()

	wxString	m_FrameTitle;
	wxLocale*	m_locale					= nullptr;
	wxTimer*	m_timer						= nullptr;

	wxPanel*			m_PanEditor			= nullptr;
	wxButton*			m_Btn_InsertAfter	= nullptr;
	wxButton*			m_Btn_InsertUpdate	= nullptr;
	wxButton*			m_Btn_MoveUp		= nullptr;
	wxButton*			m_Btn_MoveDn		= nullptr;
	wxButton*			m_Btn_Delete		= nullptr;
	CmdExecutorCtrl*	m_PanExec			= nullptr;
	cAnswersShow*		m_PanAnswers		= nullptr;

	wxBitmap			m_bitmap;

#if defined(USE_MAIN_LOG)
	wxTextCtrl*			m_txt_Log		= nullptr;
#endif
	cMainListCtrl*		m_lstPrgMaster	= nullptr;
	cDetailListCtrl*	m_lstPrgDetail	= nullptr;
	CmdEditorCtrl*		m_CmdEditor		= nullptr;


	long m_lastHoveredItem;
public:
	void OnMouseMotion(wxMouseEvent& event) {
		wxPoint mousePos = event.GetPosition();			// 1. Get Mouse position

		// 2. Controlla quale riga e quali flag interseca la posizione del mouse
		int flags		= 0;
		long itemIndex	= m_lstPrgDetail->HitTest(mousePos, flags);

		// 3. Verifica se il mouse si trova effettivamente sopra un elemento/testo valido
		if (itemIndex != wxNOT_FOUND && (flags & (wxLIST_HITTEST_ONITEMLABEL | wxLIST_HITTEST_ONITEM))) {

			// Ottimizzazione: aggiorna il ToolTip solo se il mouse passa a una riga diversa
			if (itemIndex != m_lastHoveredItem) {
				m_lastHoveredItem = itemIndex;

				// Recupera le informazioni della riga per comporre il ToolTip personalizzato
				int SubSys		= wxAtoi(m_lstPrgDetail->GetItemText(itemIndex, 1));// .ToInt();
				int Cmd			= wxAtoi(m_lstPrgDetail->GetItemText(itemIndex, 2));	// .ToInt();
				int PatternLen	= m_lstPrgDetail->GetItemText(itemIndex, 3).Length();
				const sSubSystem		*Ssys = SubSystem_GetByType((eSubSysAcro)SubSys);
				const sSampler_Commands	*pCmd = Command_GetByCmd((char)SubSys, (char)Cmd, PatternLen);

				wxString toolTipText = wxString::Format( "%s/%s", Ssys ? Ssys->Descr:"???", pCmd?pCmd->Descr:"???");

				m_lstPrgDetail->SetToolTip(toolTipText);
			}
		}
		else {
			// Se il mouse sta nell'area vuota della ListView, resetta lo stato e nasconde il ToolTip
			if (m_lastHoveredItem != -1) {
				m_lastHoveredItem = -1;
				m_lstPrgDetail->SetToolTip("");
			}
		}

		// Importante: permette la propagazione dell'evento ad altri gestori nativi
		event.Skip();
	}

};
