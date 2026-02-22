/*
 * 
 * 
 */

#include "stdwx.h"
#include "Master_Frame.h"

#include "wx/aboutdlg.h"
// these headers are only needed for custom about dialog
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"
#include "wx/artprov.h"

enum {
	ID_MNU_Quit = wxID_HIGHEST,
	ID_MNU_About,
	ID_TMR_TIMER,
	wxID_SQLINSERTAFT,
	wxID_SQLUPDATE,
	wxID_SQLDELETE,
	ID_Btn_MoveUp,
	ID_Btn_MoveDn,

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
	ID_LST_MASTER,
	ID_LST_PRGDETAIL,

	ID_MNU_PRGMAIN_NEW,
	ID_MNU_PRGMAIN_MODIFY,
	ID_MNU_PRGMAIN_COPY,
	ID_MNU_PRGMAIN_CLEAR,
	ID_MNU_PRGMAIN_DROP,
	ID_MNU_PRGMAIN_PRINT,
	ID_MNU_PRGMAIN_Export,

	ID_MNU_PRGMDET_EXECFROM,
	ID_MNU_PRGMDET_EXECTO,
	ID_MNU_PRGMDET_EXECSTEP,
	ID_TXT_LOG,
};


BEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MOUSE_EVENTS	(				OnMouseEvent )
//	EVT_RIGHT_DOWN		(				OnMouseEvent )
//	EVT_RIGHT_DOWN		(				OnClearLog )
//	EVT_MIDDLE_DOWN		(OnClearLog)

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
	EVT_MENU			( ID_MNU_PRGMAIN_NEW,		OnMenu)
	EVT_MENU			( ID_MNU_PRGMAIN_MODIFY,	OnMenu)
	EVT_MENU			( ID_MNU_PRGMAIN_COPY,		OnMenu)
	EVT_MENU			( ID_MNU_PRGMAIN_CLEAR,		OnMenu )
	EVT_MENU			( ID_MNU_PRGMAIN_DROP,		OnMenu )
	EVT_MENU			( ID_MNU_PRGMAIN_PRINT,		OnMenu )
	EVT_MENU			( ID_MNU_PRGMAIN_Export,	OnMenu )

	EVT_MENU			(ID_MNU_PRGMDET_EXECSTEP, OnMenu)
	EVT_MENU			(ID_MNU_PRGMDET_EXECFROM, OnMenu)

	EVT_TIMER			( ID_TMR_TIMER,	OnTimer)
	EVT_BUTTON			( -1, MyFrame::OnBtnCommands )
//	EVT_PAINT			(				OnPaint )

	EVT_LIST_ITEM_SELECTED		(ID_LST_MASTER, OnListEvent)
	EVT_LIST_COL_CLICK			(ID_LST_MASTER, OnListEvent)
	EVT_LIST_COL_RIGHT_CLICK	(ID_LST_MASTER, OnListEvent)
	EVT_LIST_ITEM_RIGHT_CLICK	(ID_LST_MASTER, OnListEvent)
	EVT_LIST_ITEM_SELECTED		(ID_LST_MASTER, OnListEvent)
	EVT_LIST_ITEM_DESELECTED	(ID_LST_MASTER, OnListEvent)

	EVT_LIST_ITEM_RIGHT_CLICK	( ID_LST_PRGDETAIL,	OnListEvent )
	EVT_LIST_ITEM_SELECTED		( ID_LST_PRGDETAIL,	OnListEvent )
	EVT_LIST_ITEM_DESELECTED	( ID_LST_PRGDETAIL,	OnListEvent )

END_EVENT_TABLE()

void MyFrame::OnClearLog(wxMouseEvent& Evt) {
	m_txt_Log->SetValue("");
}

void MyFrame::OnBtnCommands( wxCommandEvent&	event ) {
	wxButton* btn = static_cast<wxButton*>(event.GetEventObject());
	int EvtId = btn->GetId();
	int	id_b = event.GetId();
	wxEventType	EvTyp = event.GetEventType();

	switch (event.GetId()) {
		case wxID_SQLINSERTAFT:
		case wxID_SQLUPDATE:
			{
				cCmdStepper	s = m_CmdEditor->UI2DBData();
				s.m_MasterId = -99;

				// --- In the Editor there is not the MasterId ------------------------------------------
				wxString		OldName;
				unsigned int	ProgId;
				long			CurrItemIdx = m_lstPrgMaster->GetCurrRow(&ProgId, &OldName);
				if (CurrItemIdx >= 0) {
					s.m_MasterId = ProgId;
				}
				// ---------------------------------------------
				// --- In the Editor there is not the DetailProg ------------------------------------------
				s.m_DetailProg = m_lstPrgDetail->GetSelectedItem().m_DetailProg;
				// ---------------------------------------------
#ifdef WERTWWW
				LogMe(wxString::Format("(wxID_SQLINSERTAFT) %s\n", m_CmdEditor->UI2String()), true);
				LogMe(wxString::Format("\tIds.....: %ld/%ld\n", s.m_MasterId, s.m_DetailProg), true);
				LogMe(wxString::Format("\tMotor...: %d\n", s.m_Motor), true);
				LogMe(wxString::Format("\tCmd.....: '%c'\n", (char)s.m_Cmd), true);
				LogMe(wxString::Format("\tPattern.: '%s' (%d)\n", s.m_Pattern, s.m_Cnt), true);
				wxString dataStr;
				for (size_t i = 0; i < WXSIZEOF(s.m_Par); ++i) {
					dataStr << s.m_Par[i] << (i < (WXSIZEOF(s.m_Par)-1) ? ", " : "");
				}
				LogMe(wxString::Format("\tData....: %s\n", dataStr), true);
#endif
				m_lstPrgDetail->UpdateItem(s, EvtId == wxID_SQLINSERTAFT);

			}
			break;
		case wxID_SQLDELETE:
			m_lstPrgDetail->DeleteItem();
			break;
		case ID_Btn_MoveUp:
		case ID_Btn_MoveDn:		m_lstPrgDetail->SwapItem( EvtId == ID_Btn_MoveDn);	break;

		default:
			LogMe(wxString::Format("(???) Id1=%d, Id2=%d, Event=%0X\n", EvtId, id_b, EvTyp), true);
			break;
	}
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
	/*
	m_menuPopUp = new wxMenu("Title");
	m_menuPopUp->Append( ID_MNU_About, _("&About...") );
	m_menuPopUp->AppendSeparator();
	*/
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
	m_PanEditor = new wxPanel(this);
	m_Btn_InsertAfter = new wxButton(m_PanEditor, wxID_SQLINSERTAFT, _("Insert (After)"));
	m_Btn_InsertUpdate = new wxButton(m_PanEditor, wxID_SQLUPDATE, _("Update"));
	m_Btn_Delete = new wxButton(m_PanEditor, wxID_SQLDELETE, _("Delete"));
	m_Btn_MoveUp = new wxButton(m_PanEditor, ID_Btn_MoveUp, _("Up"));
	m_Btn_MoveDn = new wxButton(m_PanEditor, ID_Btn_MoveDn, _("Down"));

	m_Btn_InsertAfter->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_SAVE, wxART_BUTTON), wxLEFT);
	m_Btn_InsertUpdate->SetBitmap(wxArtProvider::GetIcon(wxART_FIND_AND_REPLACE, wxART_BUTTON), wxLEFT);
	m_Btn_Delete->SetBitmap(wxArtProvider::GetIcon(wxART_DELETE, wxART_BUTTON), wxLEFT);
	m_Btn_MoveUp->SetBitmap(wxArtProvider::GetIcon(wxART_GO_UP, wxART_BUTTON), wxLEFT);
	m_Btn_MoveDn->SetBitmap(wxArtProvider::GetIcon(wxART_GO_DOWN, wxART_BUTTON), wxLEFT);


	{
		wxBoxSizer* SizButtons = new wxBoxSizer(wxVERTICAL);
		SizButtons->Add(0, 0, 1, wxGROW, 0);
		SizButtons->Add(m_Btn_InsertAfter, 0, wxALL | wxGROW, 0);
		SizButtons->Add(m_Btn_InsertUpdate, 0, wxALL | wxGROW, 0);
		SizButtons->Add(m_Btn_MoveUp, 0, wxALL | wxGROW, 0);
		SizButtons->Add(m_Btn_MoveDn, 0, wxALL | wxGROW, 0);
		SizButtons->Add(0, 0, 1, wxGROW, 0);
		SizButtons->Add(m_Btn_Delete, 0, wxALL | wxGROW, 0);
		SizButtons->Add(0, 0, 1, wxGROW, 0);

		m_PanEditor->SetSizer(SizButtons);
		m_PanEditor->Layout();
	}
	//------------------------------------------------------------------
	m_Btn_InsertAfter->SetToolTip(_("Insert a new record after selected record and renumber"));
	m_Btn_InsertUpdate->SetToolTip(_("Change data of selected record"));
	m_Btn_MoveUp->SetToolTip(_("Move selected record before previous"));
	m_Btn_MoveDn->SetToolTip(_("Move selected record after next"));
	m_Btn_Delete->SetToolTip(_("Delete selected record and renumber"));

	//---------------------------------------------------------------------


	m_txt_Log		= new wxTextCtrl( this, ID_TXT_LOG,		_("Hello\n"),	wxPoint( 100,  6 ),	wxSize(50, -1 ), wxTE_MULTILINE );

	m_lstPrgMaster = new cMainListCtrl(this, ID_LST_MASTER, wxDefaultPosition, wxSize(150 + 50, 50), wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
	m_lstPrgMaster->MainPrg_Fill();

	m_lstPrgDetail = new cDetailListCtrl(this, ID_LST_PRGDETAIL, wxDefaultPosition, wxSize(48, 50), wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);

	m_CmdEditor = new CmdEditorCtrl(this, wxID_ANY);

	m_PanExec = new CmdExecutorCtrl(this);
	m_PanExec->SetEditorAndDB(m_CmdEditor, m_lstPrgDetail);

	LogMeSet(m_txt_Log);
	LogMe(wxString::Format("Working path:'%s'\n", wxFileName::GetCwd()+"\\" ), false);
	LogMe(wxString::Format("Working path:'%s'\n", wxFileName::GetCwd()+wxT("\\") ), false);

	SetLayouts();

	m_lstPrgMaster->Focus(0);
	m_lstPrgMaster->Select(0);
	m_lstPrgMaster->EnsureVisible(0);

//	if ( !m_bitmap.LoadFile( _T("C:/DatiBaldo/Job/Video/Minimal_Video/Jigsaw_06/Images/Src/bmpExample.bmp"), wxBITMAP_TYPE_BMP ) )
//		wxLogError(wxT("Can't load BMP image"));
	//---------------------------------------------------
}

void MyFrame::SetLayouts ( void ) {
#ifdef USE_AUI
	m_mgr.AddPane(m_lstPrgMaster,	wxAuiPaneInfo().Name(wxT("m_lstPrgMaster"))	.Caption(_("DBMaster"))		.Left()			.PaneBorder(false));
	m_mgr.AddPane(m_lstPrgDetail,	wxAuiPaneInfo().Name(wxT("m_lstPrgDetail"))	.Caption(_("Detail"))		.CenterPane()		.PaneBorder(false));
	m_mgr.AddPane(m_CmdEditor,		wxAuiPaneInfo().Name(wxT("m_CmdEditor"))	.Caption(_("m_CmdEditor"))	.Bottom()		.PaneBorder(false));

	m_mgr.AddPane(m_PanEditor,		wxAuiPaneInfo().Name(wxT("m_PanEditor"))	.Caption(_("Editor"))		.Right()		.PaneBorder(false));
	m_mgr.AddPane(m_txt_Log,		wxAuiPaneInfo().Name(wxT("m_txt_Log"))		.Caption(_("m_txt_Log"))	.Right()		.PaneBorder(false));
	m_mgr.AddPane(m_PanExec,		wxAuiPaneInfo().Name(wxT("m_PanExec"))		.Caption(_("Execution"))	.Right()		.PaneBorder(false));

	m_mgr.SetManagedWindow ( this );
	AuiRefresh ();
	wxCommandEvent Evt; Evt.SetId(ID_MNU_Perspect_Load);OnMenu(Evt);
#else
	wxBoxSizer* sizLogs = new wxBoxSizer ( wxVERTICAL );
	sizLogs->Add ( m_txt_Log, 1, wxALL | wxEXPAND, 2 );

	wxBoxSizer* sizButtons = new wxBoxSizer ( wxVERTICAL );
	sizButtons->Add ( m_btn_Start, 0, wxALL | wxEXPAND, 2 );
	sizButtons->Add ( m_btn_Stop, 0, wxALL, 2 );

	wxBoxSizer* sizMain = new wxBoxSizer ( wxHORIZONTAL );
	sizMain->Add ( sizLogs, 1, wxEXPAND, 2 );
	sizMain->Add ( sizButtons, 0, wxEXPAND, 2 );

	this->SetSizer ( sizMain );
	this->Layout ();
#endif
}

MyFrame::~MyFrame() {
	wxDELETE(m_locale);
	//wxDELETE(m_menuPopUp);
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
			}
			break;
		case ID_MNU_PRGMAIN_NEW:
			m_lstPrgMaster->DBCreateNewProcess();
			break;
		case ID_MNU_PRGMAIN_MODIFY:
		case ID_MNU_PRGMAIN_COPY:
			{
				wxString		OldName;
				unsigned int	ProgId;
				long			CurrItemIdx = m_lstPrgMaster->GetCurrRow(&ProgId, &OldName);
				if (CurrItemIdx >= 0) {
					m_lstPrgMaster->DBModifyCopyProcess(OldName, ProgId, event.GetId() == ID_MNU_PRGMAIN_MODIFY);
					m_lstPrgMaster->Reload(CurrItemIdx);
				}
			}
			break;
		case ID_MNU_PRGMAIN_CLEAR:
		case ID_MNU_PRGMAIN_DROP:
			{
				wxString		OldName;
				unsigned int	ProgId;
				long			CurrItemIdx = m_lstPrgMaster->GetCurrRow(&ProgId, &OldName);
				if (CurrItemIdx >= 0) {
					m_lstPrgMaster->DBClearProcess(OldName, ProgId, event.GetId() == ID_MNU_PRGMAIN_DROP);
					m_lstPrgMaster->Reload(CurrItemIdx);
				}
			}
			break;
		case ID_MNU_PRGMAIN_PRINT:
		case ID_MNU_PRGMAIN_Export:
			{
				wxString		OldName;
				unsigned int	ProgId;
				long			CurrItemIdx = m_lstPrgMaster->GetCurrRow(&ProgId, &OldName);
				if (CurrItemIdx >= 0) {
					m_lstPrgMaster->DBPrintExport(OldName, ProgId, event.GetId() == ID_MNU_PRGMAIN_PRINT);
					m_lstPrgMaster->Reload(CurrItemIdx);
				}
			}
			break;
		case ID_MNU_PRGMDET_EXECSTEP:
			{
				long itemIndex = m_lstPrgDetail->GetFirstSelected();
				m_PanExec->ExecuteFrom(itemIndex, itemIndex+1);
			}
		break;
		case ID_MNU_PRGMDET_EXECFROM:
			{
				long itemIndex = m_lstPrgDetail->GetFirstSelected();
				m_PanExec->ExecuteFrom(itemIndex, m_lstPrgDetail->GetItemCount());
			}
			break;

/*		case ID_MNU_PRGMAIN_SORT:
			m_lstPrgMaster->ChangeSort();	//m_SortByName = !m_SortByName;
			m_lstPrgMaster->MainPrg_Fill();
			break;
*/


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
								wxString Persp("layout3|name=m_lstPrgMaster;caption=DBMaster;state=2098684;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=421;besth=204;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1;floatw_cli=-1;floath_cli=-1|name=m_lstPrgDetail;caption=Detail;state=2098684;dir=5;layer=0;row=0;pos=0;prop=141395;bestw=404;besth=204;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1;floatw_cli=-1;floath_cli=-1|name=m_CmdEditor;caption=m_CmdEditor;state=2098684;dir=3;layer=0;row=0;pos=0;prop=178300;bestw=20;besth=20;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1;floatw_cli=-1;floath_cli=-1|name=m_PanEditor;caption=Editor;state=2098684;dir=2;layer=0;row=0;pos=0;prop=100000;bestw=110;besth=145;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1;floatw_cli=-1;floath_cli=-1|name=m_txt_Log;caption=m_txt_Log;state=256;dir=5;layer=0;row=0;pos=1;prop=58605;bestw=158;besth=128;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1;floatw_cli=-1;floath_cli=-1|name=m_PanExec;caption=Execution;state=2098684;dir=3;layer=0;row=0;pos=1;prop=21700;bestw=96;besth=93;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=1414;floaty=870;floatw=-1;floath=-1;floatw_cli=96;floath_cli=93|dock_size(4,0,0)=294|dock_size(5,0,0)=294|dock_size(3,0,0)=259|dock_size(2,0,0)=179|");
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
	/*
	if( m_menuPopUp!=NULL && event.Button(wxMOUSE_BTN_RIGHT) ){
		PopupMenu(m_menuPopUp, event.GetPosition());
	}
	*/

	wxString	Str = wxString::Format("%d, %d (%d) - ", pt.x, pt.y, event.GetWheelRotation() );

	if( event.LeftIsDown() )	Str += _("Left ");
	if( event.MiddleIsDown() )	Str += _("Middle ");
	if( event.RightIsDown() )	Str += _("Right ");

	wxSize s = this->GetSize();
	Str += wxString::Format("size %dx%d", s.x, s.y);

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

void MyFrame::OnListEvent(wxListEvent& evt) {
	int			Id = evt.GetId();
	wxEventType	EvTyp = evt.GetEventType();
	switch (Id) {
		case ID_LST_MASTER:
			if (EvTyp == wxEVT_LIST_ITEM_SELECTED) {
				long MasterId = (evt.m_itemIndex >= 0) ? wxAtol(evt.GetItem().GetText()) : -1;
				m_CmdEditor->SetDbInfo(MasterId, -1);
				m_lstPrgDetail->PrgDetail_Fill(MasterId);
				m_lstPrgDetail->Focus(0);	m_lstPrgDetail->Select(0);
			
				m_lstPrgMaster->SetFocus();


			} else if (EvTyp == wxEVT_LIST_ITEM_RIGHT_CLICK
				|| EvTyp == wxEVT_LIST_COL_RIGHT_CLICK) {
				wxMenu* m_menuPopUp = new wxMenu;
				m_menuPopUp->Append(ID_MNU_PRGMAIN_NEW,		_("New Process"));
				m_menuPopUp->Append(ID_MNU_PRGMAIN_MODIFY,	_("Rename Process"));
				m_menuPopUp->Append(ID_MNU_PRGMAIN_COPY,	_("Duplicate Process"));
				m_menuPopUp->AppendSeparator();
				m_menuPopUp->Append(ID_MNU_PRGMAIN_CLEAR,	_("Remove Steps"));
				m_menuPopUp->Append(ID_MNU_PRGMAIN_DROP,	_("Remove Process!"));
				m_menuPopUp->AppendSeparator();
				m_menuPopUp->Append(ID_MNU_PRGMAIN_PRINT,	_("Print"));
				m_menuPopUp->Append(ID_MNU_PRGMAIN_Export,	_("Export"));

				PopupMenu(m_menuPopUp, wxDefaultPosition); //event.GetPosition());
				wxDELETE(m_menuPopUp);
			}else if(EvTyp==wxEVT_LIST_COL_CLICK	) {
				m_lstPrgMaster->ChangeSort();
				m_lstPrgMaster->MainPrg_Fill();
			}
			break;
		case ID_LST_PRGDETAIL:
			if (EvTyp == wxEVT_LIST_ITEM_SELECTED) {
				wxListItem info;
				info.m_itemId = m_lstPrgDetail->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
				info.SetTextColour(*wxRED);	//info.SetBackgroundColour(*wxGREEN);
				info.SetFont(*wxITALIC_FONT);
				m_lstPrgDetail->SetItem(info);
				m_lstPrgDetail->RefreshItem(info.m_itemId);
				//-----------------------------------------------------------------------
				//info.m_mask = wxLIST_MASK_TEXT;	//I want the text!
				//evt.Skip();

				cCmdStepper	vStep = m_lstPrgDetail->GetSelectedItem();
				m_CmdEditor->Freeze();
				//............................................
				m_CmdEditor->DBData2UI(vStep);
				m_CmdEditor->SetDbInfo(vStep.m_MasterId, vStep.m_DetailProg);

				//.....................................................
				m_CmdEditor->Thaw();

				//-----------------------------------------------------------------------

			} else if (EvTyp == wxEVT_LIST_ITEM_DESELECTED) {
				wxListItem info;
				info.m_itemId = evt.m_itemIndex;
				info.SetTextColour(*wxBLACK);
				info.SetFont(*wxNORMAL_FONT);
				m_lstPrgDetail->SetItem(info);
			} else if (EvTyp == wxEVT_LIST_ITEM_RIGHT_CLICK) {
				if (evt.m_itemIndex >= 0) {
					wxListItem info;
					info.m_itemId = evt.m_itemIndex;
					info.SetTextColour(*wxBLUE);
					info.SetFont(*wxITALIC_FONT);
					m_lstPrgDetail->SetItem(info);
				}
				//evt.Skip();
				//.............................................................
				wxMenu* m_menuPopUp;
				m_menuPopUp = new wxMenu;
				//m_menuPopUp->Append( ID_MNU_PRGMDET_EXECTO,		_( "Execute Until here") );
				m_menuPopUp->Append(ID_MNU_PRGMDET_EXECFROM, _("(Re)Start From here"));
				m_menuPopUp->Append(ID_MNU_PRGMDET_EXECSTEP, _("Execute Single Step"));
				//-----------------------------------------------------------------
				PopupMenu(m_menuPopUp, wxDefaultPosition); //event.GetPosition());
				wxDELETE(m_menuPopUp);
				//---------------------------------------------------------------------

				//.............................................................
			}
			break;
	}
}