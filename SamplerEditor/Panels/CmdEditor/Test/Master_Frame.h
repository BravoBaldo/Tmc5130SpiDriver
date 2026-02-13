//Master_Frame.h
#ifndef _MASTER_FRAME_H_
#define _MASTER_FRAME_H_

#include "wx/wx.h"
#include "../CmdEditorCtrl.h"

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
//	void OnRefresh		()

	void OnMenu			( wxCommandEvent&	Evt );
	void OnBtnCommands	( wxCommandEvent&	Evt );
//	void OnChoice		( wxCommandEvent&	Evt );
	void OnTextInput	( wxCommandEvent&	Evt);

	void SetLayouts ( void );

#ifdef USE_AUI
	wxString	Perspective_Get	( void )			{ return m_mgr.SavePerspective(); }
	void		Perspective_Set	(const wxString& p)	{ 
														if(p.Len()>0){
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
	wxTimer*	m_timer;
	wxLocale*	m_locale;

	wxButton*	m_btn_Start;
	wxButton*	m_btn_Stop;
	wxTextCtrl* m_txt_Input;
	wxTextCtrl*	m_txt_Log;
	wxBitmap	m_bitmap;
	wxMenu*		m_menuPopUp;

	CmdEditorCtrl* m_CmdEditorPanel;
};

#endif