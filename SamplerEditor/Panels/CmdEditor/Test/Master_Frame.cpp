/*
 * 
 * 
 */

#include "stdwx.h"
#include "Master_Frame.h"
#include "../CmdEditorCtrl.h"


#include "wx/aboutdlg.h"
// these headers are only needed for custom about dialog
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"


enum {
	ID_MNU_Quit		= wxID_HIGHEST,
	ID_MNU_About,
	ID_TMR_TIMER,
	ID_BTN_Start,
	ID_BTN_Stop,
	ID_TXT_Input,
#ifdef USE_AUI
	ID_MNU_Perspect,
	ID_MNU_Perspect_Save,
	ID_MNU_Perspect_Load,
	ID_MNU_Perspect_Clear,
	ID_MNU_Perspect_Fact,
	ID_MNU_Perspect_Warm,
	ID_MNU_Perspect_ToFile,
	ID_MNU_Perspect_ToClip,
	ID_MNU_Perspect_FromClip,
#endif
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MOUSE_EVENTS	(				OnMouseEvent )
//	EVT_RIGHT_DOWN		(				OnMouseEvent )
	EVT_MENU			( ID_MNU_Quit,	OnMenu )
	EVT_MENU			( ID_MNU_About,	OnMenu )
#ifdef USE_AUI
	EVT_MENU			( ID_MNU_Perspect_Save,		OnMenu )
	EVT_MENU			( ID_MNU_Perspect_Load,		OnMenu )
	EVT_MENU			( ID_MNU_Perspect_Clear,	OnMenu )
	EVT_MENU			( ID_MNU_Perspect_Fact,		OnMenu )
	EVT_MENU			( ID_MNU_Perspect_Warm,		OnMenu )
	EVT_MENU			( ID_MNU_Perspect_ToFile,	OnMenu )
	EVT_MENU			( ID_MNU_Perspect_ToClip,	OnMenu )
	EVT_MENU			( ID_MNU_Perspect_FromClip,	OnMenu )
#endif


	EVT_TIMER			( ID_TMR_TIMER,	OnTimer)
	EVT_BUTTON			( ID_BTN_Start,	OnBtnCommands )
	EVT_BUTTON			( ID_BTN_Stop,	OnBtnCommands )
//	EVT_PAINT			(				OnPaint )

//	EVT_CHOICE			(wxID_ANY, OnChoice)
	EVT_TEXT			(ID_TXT_Input, OnTextInput)

END_EVENT_TABLE()

void MyFrame::OnTextInput(wxCommandEvent& ) {
	wxString s = m_txt_Input->GetValue();
	m_CmdEditorPanel->String2UI(s);
}

void MyFrame::OnBtnCommands( wxCommandEvent&	event ) {
	m_btn_Start->Enable( false );
	m_btn_Stop->Enable( false );

	switch( event.GetId() ) {
		case ID_BTN_Start:	
			LogMe("String2UI\n", true);
			m_CmdEditorPanel->String2UI("m0;G,100,120,2000");
			break;
		case ID_BTN_Stop:
			LogMe("ToDo!!!!\n",	true );
			break;
		default:			LogMe("Unknown\n",	true );			break;
	}

	m_btn_Start->Enable( true );
	m_btn_Stop->Enable( true );
#ifdef USE_AUI
	AuiRefresh();
#endif
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, wxLocale* locale)
	: wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
	SetBackgroundColour( wxColour(236, 233, 216) );
	SetBackgroundColour( wxColour(88, 100, 255) );
	SetBackgroundColour( wxColour(120, 122, 255) );
	wxIcon icon = wxIcon(wxT("Minimal_icon")); //See .rc file
	SetIcon(icon);

	m_locale			= locale;
	m_FrameTitle		= title;
	m_menuPopUp			= NULL;

	//---------------------------------------------------
	// Menu
	//---------------------------------------------------
	wxMenu *menuFile	= new wxMenu;
		menuFile->Append( ID_MNU_About, _("&About...") );
		menuFile->AppendSeparator();
		menuFile->Append( ID_MNU_Quit, _("E&xit") );

	wxMenuBar *menuBar	= new wxMenuBar;
		menuBar->Append( menuFile, _("&File") );

#ifdef USE_AUI
	wxMenu *menuPerspect	= new wxMenu;
		menuPerspect->Append( ID_MNU_Perspect_Save,		_("Save"	) );
		menuPerspect->Append( ID_MNU_Perspect_Load,		_("Load"	) );
		menuPerspect->Append( ID_MNU_Perspect_Fact,		_("Factory"	"\tF9") );
		menuPerspect->Append( ID_MNU_Perspect_Warm,		_("Warming"	) );
		menuPerspect->Append( ID_MNU_Perspect_Clear,	_("Clear"	) );
		menuPerspect->Append( ID_MNU_Perspect_ToFile,	_("To File"	) );
		menuPerspect->Append( ID_MNU_Perspect_ToClip,	_("Save Layout To Clipboard")	+"\tCtrl+C"	);
		menuPerspect->Append( ID_MNU_Perspect_FromClip,	_("Load Layout From Clipboard")	+"\tCtrl+V"	);
	wxMenu *menuTools	= new wxMenu;
	menuTools->Append( ID_MNU_Perspect,				_("Layout..."), menuPerspect );

		menuBar->Append( menuTools, _("&Tools") );
#endif


	SetMenuBar( menuBar );

	// Menu contestuale
	m_menuPopUp = new wxMenu("Title");
	m_menuPopUp->Append( ID_MNU_About, _("&About...") );
	m_menuPopUp->AppendSeparator();

	//---------------------------------------------------
	// Status Bar
	//---------------------------------------------------
	CreateStatusBar( 3 );
	SetStatusText( _("Welcome to ") + m_FrameTitle );

	//---------------------------------------------------
	// Timer
	//---------------------------------------------------
	m_timer = new wxTimer(this, ID_TMR_TIMER);
	m_timer->Start(100);	// millisecond interval

	//---------------------------------------------------
	// Widgets
	//---------------------------------------------------
	m_btn_Start		= new wxButton	( this, ID_BTN_Start,	_( "Start" ),	wxPoint( 350,  5 ),	wxDefaultSize );
	m_btn_Stop		= new wxButton	( this, ID_BTN_Stop,	_( "Stop" ),	wxPoint( 350, 35 ),	wxDefaultSize );

	m_txt_Input		= new wxTextCtrl(this, ID_TXT_Input, _("m3;y,101,102,103"), wxPoint(100, 6), wxSize(50, -1));

	m_txt_Log		= new wxTextCtrl( this, wxID_ANY,		_("Hello\n"),	wxPoint( 100,  6 ),	wxSize(50, -1 ), wxTE_MULTILINE );
	m_CmdEditorPanel= new CmdEditorCtrl(this, wxID_ANY);
	LogMeSet(m_txt_Log);
	LogMe(wxString::Format("Working path:'%s'\n", wxFileName::GetCwd()+"\\" ), false);
	LogMe(wxString::Format("Working path:'%s'\n", wxFileName::GetCwd()+wxT("\\") ), false);




	SetLayouts ();

	
//	if ( !m_bitmap.LoadFile( _T("C:/DatiBaldo/Job/Video/Minimal_Video/Jigsaw_06/Images/Src/bmpExample.bmp"), wxBITMAP_TYPE_BMP ) )
//		wxLogError(wxT("Can't load BMP image"));
	//---------------------------------------------------
}

void MyFrame::SetLayouts ( void ) {
#ifdef USE_AUI	
	m_mgr.AddPane (m_CmdEditorPanel, wxAuiPaneInfo ().Name ( wxT ( "m_CmdEditorPanel" ) )	.Caption ( _( "m_CmdEditorPanel" ) )	.Top ()	);
	m_mgr.AddPane (m_txt_Input,		 wxAuiPaneInfo ().Name ( wxT ( "m_txt_Input" ) )		.Caption ( _( "m_txt_Input" ) )			.Bottom ()	);
	m_mgr.AddPane ( m_txt_Log,		 wxAuiPaneInfo ().Name ( wxT ( "m_txt_Log" ) )			.Caption ( _( "m_txt_Log" ) )			.Bottom ()	);
	m_mgr.AddPane ( m_btn_Start,	 wxAuiPaneInfo ().Name ( wxT ( "m_btn_Start" ) )		.Caption ( _( "m_btn_Start" ) )		.Right ()	);
	m_mgr.AddPane(m_btn_Stop,		 wxAuiPaneInfo ().Name ( wxT ("m_btn_Stop" ) )          .Caption ( _("m_btn_Stop")).Right());
	

	m_mgr.SetManagedWindow ( this );
	AuiRefresh ();
	wxCommandEvent Evt; Evt.SetId(ID_MNU_Perspect_Load);OnMenu(Evt);
#else
	wxBoxSizer* sizCmdEditorPanel = new wxBoxSizer(wxVERTICAL);
		sizCmdEditorPanel->Add(m_txt_Input, 0, wxALL | wxEXPAND, 2);
		sizCmdEditorPanel->Add(m_CmdEditorPanel, 1, wxALL | wxEXPAND, 2);

	wxBoxSizer* sizLogs = new wxBoxSizer ( wxVERTICAL );
		sizLogs->Add ( m_txt_Log, 1, wxALL | wxEXPAND, 2 );

	wxBoxSizer* sizButtons = new wxBoxSizer ( wxVERTICAL );
		sizButtons->Add ( m_btn_Start, 0, wxALL | wxEXPAND, 2 );
		sizButtons->Add ( m_btn_Stop, 0, wxALL, 2 );

	wxBoxSizer* sizWide = new wxBoxSizer(wxHORIZONTAL);
		sizWide->Add(sizCmdEditorPanel, 1, wxEXPAND, 2);
		sizWide->Add(sizButtons, 0, wxEXPAND, 2);

	wxBoxSizer* sizMain = new wxBoxSizer (wxVERTICAL);
		sizMain->Add (sizWide, 1, wxEXPAND, 2 );
		sizMain->Add ( sizLogs, 1, wxEXPAND, 2 );

	this->SetSizer ( sizMain );
	this->Layout ();
#endif
}

MyFrame::~MyFrame() {
	wxDELETE(m_locale);
	wxDELETE(m_menuPopUp);
	wxDELETE(m_timer);
#ifdef USE_AUI
	m_mgr.UnInit();
#endif
}

void MyFrame::OnMenu( wxCommandEvent& event ) {
	switch( event.GetId() ){
		case ID_MNU_Quit:
			Close(true);
			break;
		case ID_MNU_About:
			{
				wxAboutDialogInfo info;
				//--For Not simple version (wxGenericAboutBox)--
				//wxIcon BadIcon;	info.SetIcon ( BadIcon );	// Avoid default icon
				if(!GetIcon ().IsOk ())	info.SetIcon ( wxICON ( "Minimal_icon" ) );	// default	Form icon
				info.SetWebSite ( wxT ( "http://www.MySite.com/" ), wxT ( "My web site" ) );
				info.SetLicence ( "This is for my use only" );

				//--For Simple (wxMessageBox)--
				info.SetName ( wxT ( PRG_LONG_NAME ) );
				info.SetVersion ( wxString::Format ( "\nDate: '%s %s'" "\nWxVer:'%s'", __TDATE__, __TTIME__, wxVERSION_NUM_DOT_STRING ) );

				//info.SetDescription ( wxT ( "This is my start program" ) );
				info.SetDescription ( wxString::Format(
					"This is my start program\n"
					"Compiled at '%s %s'\n"
					"Using '%s'\n"
					, __TDATE__, __TTIME__, wxVERSION_STRING

				) );

				info.SetCopyright ( wxT ( PRG_COPYRIGHT ) );
				info.AddDeveloper ( wxT ( "Baldassarre Cesarano" ) );
				//info.AddDocWriter ( wxT ( "Doc Writer 1" ) );
				//info.AddTranslator ( wxT ( "A Translator" ) );
				//info.AddArtist ( _ ( "Leonardo Da Vinci" ) );

				

				wxAboutBox ( info, this );

				/*
				wxString wxText(PRG_LONG_NAME);								//Prog Name
				wxText += "\n (c) Copyright 2008-2016, ...  ";				//Date
				wxText << wxString::Format ( "\n\n %s", wxVERSION_STRING );	//WxWidgets version
				wxText << wxString::Format ( "\nCompiled: %s %s", __TDATE__, __TTIME__ );	//Compilation Date
				wxMessageBox ( _( wxText ), _( PRG_UNIQUE_FIRM ), wxOK | wxICON_INFORMATION, this );
				*/

				//wxMessageBox ( wxString ( _ ( "About" ) ) << " '" << m_FrameTitle << "'", m_FrameTitle, wxOK | wxICON_INFORMATION, this );
			}
			break;
#ifdef USE_AUI
		//case ID_MNU_Perspect:
		case ID_MNU_Perspect_Save:
		case ID_MNU_Perspect_Load:
		case ID_MNU_Perspect_Clear:
		case ID_MNU_Perspect_Fact:
		case ID_MNU_Perspect_Warm:
		case ID_MNU_Perspect_ToFile:
		case ID_MNU_Perspect_ToClip:
		case ID_MNU_Perspect_FromClip:
			{
				switch( event.GetId() ){
					case ID_MNU_Perspect_Save:	Perspect_Config.Write		(Perspect_Key, this->Perspective_Get() );	break;
					case ID_MNU_Perspect_Clear:	Perspect_Config.DeleteEntry	(Perspect_Key, false);						break;
					case ID_MNU_Perspect_Load:
						{
							wxString Perspect;	Perspect_Config.Read(Perspect_Key, &Perspect, "" );	//true if value was really read, false if default used
							if(Perspect.Len()>0)	this->Perspective_Set( Perspect );
						}
						break;
					case ID_MNU_Perspect_ToFile:
						{
							wxFile file;
							if (file.Open(Perspect_FileName, wxFile::write_append)){
								file.Write( wxString::Format("%s\n", this->Perspective_Get()) );
								file.Flush();
								file.Close();
							}
						}
						break;
						case ID_MNU_Perspect_Fact:
							{
								wxString Persp("layout2|name=m_txt_Log;caption=m_txt_Log;state=2099196;dir=5;layer=0;row=0;pos=0;prop=100000;bestw=29;besth=94;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=m_btn_Start;caption=m_btn_Start;state=2099196;dir=2;layer=0;row=0;pos=0;prop=100000;bestw=88;besth=26;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|name=m_btn_Stop;caption=m_btn_Stop;state=2099196;dir=2;layer=0;row=0;pos=1;prop=100000;bestw=88;besth=26;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1|dock_size(5,0,0)=31|dock_size(2,0,0)=90|");
								if(Persp.Len()>0)	this->Perspective_Set( Persp );
							}
							break;
					case ID_MNU_Perspect_ToClip:
					case ID_MNU_Perspect_FromClip:
						switch (event.GetId()) {
							case ID_MNU_Perspect_FromClip:
								if (wxTheClipboard->Open()) {
									wxTextDataObject data;
									wxTheClipboard->GetData( data );
									this->Perspective_Set(data.GetText());
								}
								break;
							case ID_MNU_Perspect_ToClip:
								{
									wxString s = this->Perspective_Get();
									if (!s.IsEmpty() && wxTheClipboard->Open()) {
										wxTheClipboard->SetData(new wxTextDataObject(s));
									}
								}
								break;
						}
						if(wxTheClipboard->IsOpened())
							wxTheClipboard->Close();
						break;
				}
			}
			break;
#endif
		default:
			wxMessageBox( wxString( _("Undefined event: ") ) << event.GetId(), m_FrameTitle, wxOK | wxICON_INFORMATION, this);
			break;
	}
}

void MyFrame::OnMouseEvent(wxMouseEvent& event) {
	wxPoint		pt(event.GetPosition() );

	if( m_menuPopUp!=NULL && event.Button(wxMOUSE_BTN_RIGHT) ){
		PopupMenu(m_menuPopUp, event.GetPosition());
	}

	wxSize s = this->GetSize();

	//wxString	Str = wxString::Format("%d, %d (%d) - ", pt.x, pt.y, event.GetWheelRotation() );
	wxString	Str = wxString::Format("%d, %d (%d) - ", s.x, s.y, event.GetWheelRotation());

	if( event.LeftIsDown() )	Str += _("Left ");
	if( event.MiddleIsDown() )	Str += _("Middle ");
	if( event.RightIsDown() )	Str += _("Right ");

	SetStatusText( Str );
}

void MyFrame::OnTimer(wxTimerEvent& WXUNUSED(event) ) {
	SetStatusText( wxDateTime::UNow().Format(_T("%Y-%m-%d %H:%M:%S")), 2 );
}

void MyFrame::OnPaint(wxPaintEvent& WXUNUSED(event) ) {
	if( m_bitmap.IsOk() ){
		wxPaintDC dc( this );
		dc.DrawBitmap( m_bitmap, 0, 0, true /* use mask */ );
	}
}
