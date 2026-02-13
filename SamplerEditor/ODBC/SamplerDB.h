#pragma once

#include "wx/wx.h"
#include "wx/listctrl.h"	//ListCtrl_FillFromSql
#include <wx/dataview.h>	//wxDataViewListCtrl, wxmsw31??_adv.lib


#define OTL_ODBC // Compile OTL 4.0/ODBC
// #define OTL_ODBC_UNIX // uncomment this line if UnixODBC is used
#include <otlv4.h> // include the OTL 4.0 header file

//#define USESAMPLERDB
#ifndef DEFAULTCONNECTIONSTRING
	//#error A connection string is required
#if defined(USESAMPLERDB)
	#define DEFAULTCONNECTIONSTRING	"Driver={SQLite3 ODBC Driver};Database=C:\\Data\\Windows\\wxWidgets\\Database\\SQLite\\Sampler.db;"
#else
	#define DEFAULTCONNECTIONSTRING	"Driver={SQLite3 ODBC Driver};Database=C:\\Data\\sample-database-sqlite-1\\NazarSQLite.db;"
#endif
//
	//#define DEFAULTCONNECTIONSTRING	"DSN=Test64_00"
#endif

#ifdef OTL_UNICODE
	#define CHAR_FROMDB		OTL_UNICODE_CHAR_TYPE
	#define TOCHARDB()		wc_str()
#else
	#define CHAR_FROMDB		char
	#define TOCHARDB()		mb_str()
#endif


#if defined(USESAMPLERDB)
	#define PROGMASTER_TABLENAME "SAM_ProgMaster"
	#define PROGDETAIL_TABLENAME "SAM_ProgDetail"
#else
	#define PROGMASTER_TABLENAME "SAM_ProgMaster"
	#define PROGDETAIL_TABLENAME "SAM_ProgDetail"
#endif
#define DBIterator otl_stream


class Next_DB : public wxObject {
	DECLARE_CLASS(Next_DB)
	otl_connect			AriesConn;	 // connect object
	wxString		m_ConnectionString;

	bool			ExecuteSQL(const wxString& SqlCmd, bool AutoCommit, bool IgnoreError = false);
	otl_stream* GP_Open(const wxString& strQuerySQL);
	otl_stream* GP_Open(const wxString& Sql_Select, const wxString& Sql_From, const wxString& Sql_Where, const wxString& Sql_OrderBy, long* Count);
	bool			GP_Close(otl_stream* i);
	void			Open(void);
	void			Open(wxString ConnectionString);
	void			Close(void);
	bool			IsClosed(void);

	static wxDateTime	wxDate2Dt(const otl_datetime& dt);
	static void			ErrorShow(otl_exception& p);
	static wxString		SQLStrPrepare(wxString str);
	static wxString		SQLStrPrepare(float f);
	static otl_datetime	Dt2wxDate2(const wxDateTime& wxD);
	static void			Dt2wxDate(otl_datetime& dt, wxDateTime& wxD);
	static otl_datetime	wxDateTime2OTL(const wxDateTime& wxD);
	static wxString		Date2SqlStr(const wxDateTime& wxD);
	static wxString		DateDiff2SqlStr(const wxString& strFrom, const wxString& strTo);

	DBIterator* ProgDetail_Open(unsigned int ProgId, unsigned int DetailProg = 0);
	//	bool			ProgDetail_Iterate	( DBIterator* i, cNazarI2C & Step, long IdMethodic=-1 );
	bool			ProgDetail_Close(DBIterator* i) { return GP_Close(i); };

	int				ListCtrl_FillFromSql(wxListCtrl*		 ListCtrl, const wxString& SqlQuery, bool DoResize = true, int Fld2Translate = -1);
	int				ListCtrl_FillFromSql(wxDataViewListCtrl* ListCtrl, const wxString& SqlQuery, bool DoResize = true, int Fld2Translate = -1);

	long			ProgMaster_GetNextId(unsigned int StartIdx);
public:
	explicit		Next_DB(const wxString& ConnectionString = DEFAULTCONNECTIONSTRING);
	~Next_DB() noexcept;

	int				ProgMaster_Fill2	(wxListCtrl* ListCtrl, bool SortByName, bool DoResize = true, int Fld2Translate = 1, byte Filter = 0);
	bool			ProgMaster_Insert	(const wxString& NewProgNameIn, unsigned int Id=0);
	bool			ProgMaster_Copy		(unsigned int ProgIdOld, const wxString& NewProgName, unsigned int ProgIdNew = 0);
	bool			ProgDetail_Delete	(unsigned int ProgId, bool DelFather);
	bool			ProgMaster_Print	(unsigned int ProgId, const wxString& ProgName, const wxString& FilePathName);
	bool			ProgMaster_Export	(unsigned int ProgId, const wxString& FilePathName);

	int				ProgDetail_Fill		(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize = true, int Fld2Translate = 1);
};
