// Name:        stdwx.h
#if !defined(WX_STDWX_H)
#define WX_STDWX_H

	#define PRG_UNIQUE_FIRM					"MasterPrgMinimal"
	#define PRG_LONG_NAME					"Master Program Minimal"
	#define PRG_COPYRIGHT					"(C) Baldassarre Cesarano, 2026"

	#include "wx/wx.h"
	#include "wx/arrimpl.cpp"
	#include "wx/tokenzr.h"		//miniParser_HTTP
	#include "wx/protocol/http.h"
	#include "wx/snglinst.h"	// SingleInstance
	#include "wx/fs_mem.h"		// MemoFilesys
	#include "wx/cmdline.h"
	#include "wx/msw/private.h"
	#include "wx/config.h"

//#define USE_ODBC
//#if defined(USE_ODBC)
//	#include "SamplerDB.h"
//#else
	#include "cDBSampler.h"
	#define SQLLITEDBPATH "../Sampler.db"
//#endif
	wxString		ShowBuffer			( byte* Buffer, unsigned int LenBuf );
	void			LogMeSet			( wxTextCtrl* p );
	void			LogMe				( const wxString & ToLog, bool PrependTime );
	unsigned long	GetMaskFromString	( const wxString & strBuf );
	void			gMainFrameSet		( wxFrameBase* p );
	void			g_SetStatusText		( const wxString& text, int nField );

#if defined(USE_ODBC)
	Next_DB*	g_NazarDB_Get(void);
	void		g_NazarDB_Close(void);
#endif

	wxArrayString	RetArray(const wxString& a = "", const wxString& b = "", const wxString& c = "", const wxString& d = "", const wxString& e = ""
		, const wxString& f = "", const wxString& g = "", const wxString& h = "", const wxString& i = "", const wxString& j = ""
		, const wxString& k = "", const wxString& l = "", const wxString& m = "", const wxString& n = "", const wxString& o = ""
		, const wxString& p = "", const wxString& q = "", const wxString& r = "", const wxString& s = "", const wxString& t = ""
		, const wxString& u = "", const wxString& v = "", const wxString& w = "", const wxString& x = "", const wxString& y = ""
	);


#ifdef _DEBUG
	#define SIZER_STATDEBUG(sizName,sizLabel,sizDirection)			wxStaticBoxSizer	*sizName = new wxStaticBoxSizer( new wxStaticBox( this, -1, sizLabel ), sizDirection );
	#define SIZER_STATDEBUG2(Father,sizName,sizLabel,sizDirection)	wxStaticBoxSizer	*sizName = new wxStaticBoxSizer( new wxStaticBox( Father, -1, sizLabel ), sizDirection );

#else
	#define SIZER_STATDEBUG(sizName,sizLabel,sizDirection)			wxBoxSizer			*sizName = new wxBoxSizer( sizDirection );
	#define SIZER_STATDEBUG2(Father,sizName,sizLabel,sizDirection)	wxBoxSizer			*sizName = new wxBoxSizer( sizDirection );
#endif
#define SIZER_STATDEBUG3(sizName,sizLabel,sizDirection)			wxBoxSizer			*sizName = new wxBoxSizer( sizDirection );

#endif //WX_STDWX_H