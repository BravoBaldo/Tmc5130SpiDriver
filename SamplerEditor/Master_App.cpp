/*
 *
 *
 */

#include "stdwx.h"

//#include "vld.h"

#include "wx/wx.h"
#include "wx/cmdline.h"
#include "wx/snglinst.h"	// SingleInstance

#include "Master_Frame.h"

static const wxCmdLineEntryDesc	g_cmdLineDesc[] = {
	//kind					shortName	longName			description											type	flags
	{ wxCMD_LINE_SWITCH,	"h",	"help",			_ ( "displays help on the command line parameters" )  },
	{ wxCMD_LINE_SWITCH,	"v",	"version",		_ ( "print version" ) },
	{ wxCMD_LINE_OPTION,	"l",	"lang",			_ ( "Set languages examples :'ita', 'eng', 'fra'" ),				wxCMD_LINE_VAL_STRING },

	{ wxCMD_LINE_NONE }
};


void PurgeName ( wxString & S ) {
	if(		(S.Left ( 1 ) == "'"	&& S.Right ( 1 ) == "'")
		||	(S.Left ( 1 ) == "\""	&& S.Right ( 1 ) == "\"")
	)
		S = S.Mid ( 1, S.Length () - 2 );
}

int GetLangId ( const wxString & lang ) {
	int iLang = wxLANGUAGE_DEFAULT;
	if(lang.IsSameAs ( "eng", false ))	iLang = wxLANGUAGE_ENGLISH;
	else if(lang.IsSameAs ( "ita", false ))	iLang = wxLANGUAGE_ITALIAN;
	else if(lang.IsSameAs ( "fra", false ))	iLang = wxLANGUAGE_FRENCH;
	/*	else if( lang.IsSameAs( "dan", false ) )	iLang = wxLANGUAGE_DANISH;
		else if( lang.IsSameAs( "fin", false ) )	iLang = wxLANGUAGE_FINNISH;
		else if( lang.IsSameAs( "gre", false ) )	iLang = wxLANGUAGE_GREEK;
		else if( lang.IsSameAs( "isl", false ) )	iLang =
		else if( lang.IsSameAs( "nor", false ) )	iLang =
		else if( lang.IsSameAs( "ned", false ) )	iLang =
		else if( lang.IsSameAs( "por", false ) )	iLang = wxLANGUAGE_PORTUGUESE;
		else if( lang.IsSameAs( "rus", false ) )	iLang = wxLANGUAGE_RUSSIAN
		else if( lang.IsSameAs( "esp", false ) )	iLang = wxLANGUAGE_SPANISH;
		else if( lang.IsSameAs( "sve", false ) )	iLang = wxLANGUAGE_SWEDISH;
		else if( lang.IsSameAs( "deu", false ) )	iLang = wxLANGUAGE_GERMAN;
	*/
	return iLang;
}


class MyApp : public wxApp {
public:
	MyApp ();
	~MyApp ();
private:
	virtual	bool	OnInit ();
	void	SelectLanguage ( int lang );			// Recreates m_locale according to lang
	wxLocale*					m_locale;
	wxSingleInstanceChecker*	m_checker;
};

IMPLEMENT_APP ( MyApp )


MyApp::MyApp () {
	m_locale = NULL;
	m_checker = NULL;
}

MyApp::~MyApp () {
	wxDELETE ( m_checker );
#if defined(USE_ODBC)
	g_NazarDB_Close();
#endif
}

void MyApp::SelectLanguage ( int lang ) {
	wxDELETE ( m_locale );
	m_locale = new wxLocale ( lang );
	wxLocale::AddCatalogLookupPathPrefix ( wxT ( "./Languages" ) );	// Add search path
	m_locale->AddCatalog ( PRG_UNIQUE_FIRM );
}


bool MyApp::OnInit () {
	m_locale = NULL;
	m_checker = NULL;

	//---------------------------------------------------
	// Parse command line
	//---------------------------------------------------
	wxCmdLineParser cmdParser ( g_cmdLineDesc, argc, argv );
	int res;
	{
		wxLogNull log;
		res = cmdParser.Parse ( false );	// Pass false to suppress auto Usage() message
	}


	// Check if the user asked for command-line help
	if(res == -1 || res > 0 || cmdParser.Found ( wxT ( "h" ) )) {
		cmdParser.Usage ();
		return false;
	}
	// Check if the user asked for the version
	if(cmdParser.Found ( wxT ( "v" ) )) {
	#ifndef __WXMSW__
		wxLog::SetActiveTarget ( new wxLogStderr );
	#endif
		wxString	msg;
		wxString	date ( wxString::FromAscii ( __DATE__ ) );
		msg.Printf ( PRG_LONG_NAME ", " PRG_COPYRIGHT ", %s", (const wxChar*)date );
		wxLogMessage ( msg );
		return false;
	}

	wxString	Lang = "";
	if(cmdParser.Found ( wxT ( "l" ), &Lang )) {
	}
	PurgeName ( Lang );
	SelectLanguage ( GetLangId ( Lang ) );

	//---------------------------------------------------
	// Check unique instance *
	//---------------------------------------------------
	m_checker = new wxSingleInstanceChecker ( PRG_UNIQUE_FIRM );
	if(m_checker->IsAnotherRunning ()) {
		wxLogWarning ( _ ( "Program already running." ),
					   _ ( "Error starting program..." ),
					   wxOK | wxICON_EXCLAMATION
		);

		wxDELETE ( m_locale );
		return false;
	}

	//---------------------------------------------------
	// Global inits
	//---------------------------------------------------
	wxInitAllImageHandlers ();							// Init alla images
	//wxFileSystem::AddHandler(new wxMemoryFSHandler);	// Init MemoryFileSystem


	MyFrame *frame = new MyFrame (
		PRG_LONG_NAME,
		wxPoint ( 50, 50 ),
		wxSize ( 1150, 1000 ),
		m_locale
	);
	frame->CentreOnScreen ();
	frame->Show ( TRUE );
	SetTopWindow ( frame );
	return TRUE;

	//	return false;
}

