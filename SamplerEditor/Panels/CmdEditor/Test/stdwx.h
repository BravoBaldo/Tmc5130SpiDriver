// Name:        stdwx.h
#if !defined(WX_STDWX_H)
#define WX_STDWX_H

	#define PRG_UNIQUE_FIRM					"TestCmdEditor"
	#define PRG_LONG_NAME					"Test for CmdEditorCtrl"
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


	wxString		ShowBuffer			( byte* Buffer, unsigned int LenBuf );
	void			LogMeSet			( wxTextCtrl* p );
	void			LogMe				( const wxString & ToLog, bool PrependTime );
	unsigned long	GetMaskFromString	( const wxString & strBuf );
	void			gMainFrameSet		( wxFrameBase* p );
	void			g_SetStatusText		( const wxString& text, int nField );


#ifdef _DEBUG
#define SIZER_STATDEBUG(sizName,sizLabel,sizDirection)			wxStaticBoxSizer	*sizName = new wxStaticBoxSizer( new wxStaticBox( this, -1, sizLabel ), sizDirection );
#define SIZER_STATDEBUG2(Father,sizName,sizLabel,sizDirection)	wxStaticBoxSizer	*sizName = new wxStaticBoxSizer( new wxStaticBox( Father, -1, sizLabel ), sizDirection );
#else
#define SIZER_STATDEBUG(sizName,sizLabel,sizDirection)			wxBoxSizer			*sizName = new wxBoxSizer( sizDirection );
#define SIZER_STATDEBUG2(Father,sizName,sizLabel,sizDirection)	wxBoxSizer			*sizName = new wxBoxSizer( sizDirection );
#endif

#endif //WX_STDWX_H