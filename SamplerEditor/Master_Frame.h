//Master_Frame.h
#pragma once

#include "wx/wx.h"
#include "DBCmdView.h"
#include "CmdEditorCtrl.h"

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

	wxBitmap			m_bitmap;

	wxTextCtrl*			m_txt_Log		= nullptr;
	cMainListCtrl*		m_lstPrgMaster	= nullptr;
	cDetailListCtrl*	m_lstPrgDetail	= nullptr;
	CmdEditorCtrl*		m_CmdEditor		= nullptr;
};
