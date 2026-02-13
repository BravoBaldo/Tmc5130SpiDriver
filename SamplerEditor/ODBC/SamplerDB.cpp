#include "stdwx.h"
#include "SamplerDB.h"
#include "wx/filesys.h"
#include "wx/fs_mem.h"

IMPLEMENT_CLASS(Next_DB, wxObject)

//Display a Error Message
void Next_DB::ErrorShow(otl_exception& p) {
	wxString msg;
	msg << "Def_DB ===> " << DEFAULTCONNECTIONSTRING << "\n\n";	// print out error message
	msg << "Error  ===> " << p.msg << "\n\n";	// print out error message
	msg << "SQL    ===> " << p.stm_text << "\n\n";	// print out SQL that caused the error
	msg << "Info   ===> " << p.var_info << "\n\n";	// print out the vari
	msg << "SqlState => " << p.sqlstate << "\n\n";	// print out the vari
	wxMessageBox(msg, "SQL Error!", wxOK | wxICON_INFORMATION, NULL);
	//	wxMessageDialog(NULL,msg,"SQL Error!", wxOK | wxICON_INFORMATION );

}


void Next_DB::Close(void) {
	AriesConn.logoff(); // disconnect from the database
}

otl_stream* Next_DB::GP_Open(const wxString& strQuerySQL) {
	otl_stream* s = new otl_stream();
	try {
		s->open(2050, strQuerySQL.c_str(), AriesConn);
		return s;
	}
	catch (otl_exception& p) {
		ErrorShow(p);
	}
	return NULL;
}

otl_stream* Next_DB::GP_Open(const wxString& Sql_Select, const wxString& Sql_From, const wxString& Sql_Where, const wxString& Sql_OrderBy, long* Count) {
	if (Count) {
		*Count = 0;
		try {
			//wxString Sql_Cnt = wxString::Format(" SELECT COUNT(*) FROM %s %s", Sql_From, Sql_Where);

			wxString Sql_Cnt = wxString::Format(" SELECT COUNT(*) FROM %s ", Sql_From);
			if (!Sql_Where.Contains("WHERE"))	Sql_Cnt << " WHERE ";
			Sql_Cnt << Sql_Where;


			otl_stream	s(1, Sql_Cnt.c_str(), AriesConn);
			if (!s.eof()) {	// while not end-of-data
				s >> (*Count);
			}
		}
		catch (otl_exception& p) {
			ErrorShow(p);
		}
	}
	wxString Sql_Query = wxString::Format(" SELECT %s ", Sql_Select);
	if (!Sql_From.IsEmpty()) {
		Sql_Query << " FROM  ";
		Sql_Query << Sql_From;
	}
	if (!Sql_Where.IsEmpty()) {
		if (!Sql_Where.Contains("WHERE"))	Sql_Query << " WHERE ";
		Sql_Query << Sql_Where;
	}
	if (!Sql_OrderBy.IsEmpty()) {
		if (!Sql_OrderBy.Contains("ORDER BY"))	Sql_Query << " ORDER BY ";
		Sql_Query << Sql_OrderBy;
	}
	return GP_Open(Sql_Query);
}


bool Next_DB::GP_Close(otl_stream* i) {
	if (i) {
		i->close();
		wxDELETE(i);
	}
	return true;
}

bool Next_DB::IsClosed(void) {
	return(AriesConn.connected == 0);
}

//Open connection
void Next_DB::Open(void) {
	AriesConn.logoff(); // disconnect from the database
	try {
		AriesConn.rlogon(m_ConnectionString); // connect to the database
	}
	catch (otl_exception& p) {
		ErrorShow(p);
		exit(-1);
	}
}
void Next_DB::Open(wxString ConnectionString) {
	m_ConnectionString = ConnectionString;
	Open();
}

Next_DB::Next_DB(const wxString& ConnectionString) {
	m_ConnectionString = ConnectionString;
	otl_connect::otl_initialize(); // initialize ODBC environment
	Open();
}

Next_DB::~Next_DB() noexcept {
	Close();
}

// ************************
// *** Generic Routines ***
// ************************
//From OTL DateTime to WX
wxDateTime Next_DB::wxDate2Dt(const otl_datetime& dt) {
	wxDateTime Date;
	if (dt.day != 0 && dt.month != 0 && dt.year != 0) {
		Date.Set(
			(wxDateTime::wxDateTime_t)dt.day,
			(enum wxDateTime::Month)(dt.month - 1),
			(int)dt.year,
			(wxDateTime::wxDateTime_t)dt.hour,
			(wxDateTime::wxDateTime_t)dt.minute,
			(wxDateTime::wxDateTime_t)dt.second,
			(wxDateTime::wxDateTime_t)dt.fraction
		);
	}
	return Date;
}

wxString Next_DB::Date2SqlStr(const wxDateTime& wxD) {
#if defined(DBTYPE_SQLITE)
	return wxD.Format("'%Y-%m-%d'");	//SQLite
#else
	return wxD.Format("#%m/%d/%Y#");	//MS-Access
#endif
}

//Return strTo-strFrom
wxString Next_DB::DateDiff2SqlStr(const wxString& strFrom, const wxString& strTo) {
#if defined(DBTYPE_SQLITE)
	return wxString::Format("julianday(date(%s))-julianday(date(%s))", strFrom, strTo);	//SQLite
#else
	return wxString::Format("DateDiff('d', %s, %s)", strTo, strFrom);					//MS-Access
#endif
}


//From WX DateTime to OTL
otl_datetime Next_DB::wxDateTime2OTL(const wxDateTime& wxD) {
	otl_datetime dt;
	if (wxD.IsValid()) {
		dt.year = wxD.GetYear();
		dt.month = wxD.GetMonth() + 1;	//Jan=0
		dt.day = wxD.GetDay();
		dt.hour = wxD.GetHour();
		dt.minute = wxD.GetMinute();
		dt.second = wxD.GetSecond();
		dt.frac_precision = 0;//milliseconds	//3;// milliseconds
		dt.fraction = 0;				//AcqDate.GetMillisecond();
	}
	return dt;
}

//From WX Date to OTL
otl_datetime Next_DB::Dt2wxDate2(const wxDateTime& wxD) {
	otl_datetime dt;
	if (wxD.IsValid()) {
		dt.year = wxD.GetYear();
		dt.month = wxD.GetMonth() + 1;	//Jan=0
		dt.day = wxD.GetDay();
		dt.hour = 0;
		dt.minute = 0;
		dt.second = 0;
		dt.frac_precision = 0;// milliseconds
		dt.fraction = 0;
	}
	return dt;
}

//From WX Date to OTL
void Next_DB::Dt2wxDate(otl_datetime& dt, wxDateTime& wxD) {
	dt.year = wxD.GetYear();
	dt.month = wxD.GetMonth() + 1;	//Jan=0
	dt.day = wxD.GetDay();
	dt.hour = 0;
	dt.minute = 0;
	dt.second = 0;
	dt.frac_precision = 0;// milliseconds
	dt.fraction = 0;
}

//Handle "'" in SQL
wxString Next_DB::SQLStrPrepare(wxString str) {
	wxString	SqlStr = str;

	SqlStr.Replace("'", "''", true);
	return SqlStr;
}

//Handle dot and comma separators
wxString Next_DB::SQLStrPrepare(float f) {
	wxString	SqlStr;

	SqlStr.Printf("%f", f);
	SqlStr.Replace(",", ".", true);
	return SqlStr;
}
//==========================================================================

int Next_DB::ListCtrl_FillFromSql(	  wxDataViewListCtrl*	ListCtrl
									, const wxString&		SqlQuery
									, bool					//DoResize
									, int					Fld2Translate
) {
	otl_stream			sIn;
	otl_column_desc* colInf;
	int					numCols;

	//Get table structure
	try {
		sIn.set_all_column_types(otl_all_num2str | otl_all_date2str);
		sIn.open(2000, SqlQuery.c_str(), AriesConn);
		colInf = sIn.describe_select(numCols);	// describe the structure of the output columns of the result set.
		if (numCols <= 0) {
			wxMessageBox(_("None selected columns"), _("DB CONNECTION ERROR..."), wxOK | wxICON_EXCLAMATION);
			return -20;			// No Columns
		}
	}
	catch (otl_exception& p) {
		ErrorShow(p);
		return -20;			// No Columns
	}

	ListCtrl->Freeze();

	// Make Columns ------------------
	ListCtrl->ClearColumns();		// List Clear
	ListCtrl->DeleteAllItems();
	for (int i = 0; i < numCols; i++) {
		ListCtrl->AppendTextColumn(colInf[i].name, wxDATAVIEW_CELL_INERT
			//, (colInf[i].dbtype==otl_var_char) ? 90 : 50
			, colInf[i].dbsize * 5
			, (colInf[i].dbtype == otl_var_char) ? wxALIGN_LEFT : wxALIGN_RIGHT
		);
	}

	// Fill Rows -------------------
	long		CurrRow = 0;
	CHAR_FROMDB	Field[1001];
	wxString	Translation;
	while (!sIn.eof()) { // while not end-of-data	2910ms	400ms Query, 2850 inserimento
		wxVector<wxVariant> data;
		for (int i = 0; i < numCols; i++) {
			sIn >> Field;
			Translation = ((i == Fld2Translate) ? _(Field) : Field);
			data.push_back(Translation);
		}
		ListCtrl->AppendItem(data);
		CurrRow++;
	}
	/*if (DoResize == true) {
		for (int i = 0; i < numCols; i++) {
			//ListCtrl->GetColumn(i)->SetWidth(10);
			//ListCtrl->SetColumnWidth(i, wxLIST_AUTOSIZE);	//
		}
	}*/

	ListCtrl->Thaw();
	return 0;
}


int Next_DB::ProgMaster_Fill2(wxListCtrl* ListCtrl, bool SortByName, bool DoResize, int Fld2Translate, byte Filter) {
	wxString SqlWhere;
	switch (Filter) {
		case 1:	SqlWhere << "WHERE ProgId < 2000 ";		break;	//User
		case 2:	SqlWhere << "WHERE ProgId < 2000 ";		break;	//Service
		case 3:	SqlWhere << "WHERE ProgId >=2000 ";		break;	//Subroutines
		case 0:	//All
		default:										break;
	}
#if defined(USESAMPLERDB)
#else
#endif
	wxString	SqlCmd = wxString::Format("SELECT ProgId AS [%s], CAST(ProgName AS VARCHAR(255)) AS [%s] FROM " PROGMASTER_TABLENAME " %s ORDER BY %s "
		, _("Program Id")
		, _("Program Name")
		, SqlWhere
		, (SortByName ? "2" : "1")
	);
	return ListCtrl_FillFromSql(ListCtrl, SqlCmd, DoResize, Fld2Translate);
}

int Next_DB::ListCtrl_FillFromSql(wxListCtrl* ListCtrl
	, const wxString& SqlQuery
	, bool				DoResize
	, int				Fld2Translate
) {
	otl_stream			sIn;
	otl_column_desc* colInf;
	int					numCols;

	//Get table structure
	try {
		sIn.set_all_column_types(otl_all_num2str | otl_all_date2str);
		sIn.open(2000, SqlQuery.c_str(), AriesConn);
		colInf = sIn.describe_select(numCols);	// describe the structure of the output columns of the result set.
		if (numCols <= 0) {
			wxMessageBox(_("None selected columns"), _("DB CONNECTION ERROR..."), wxOK | wxICON_EXCLAMATION);
			return -20;			// No Columns
		}
	}
	catch (otl_exception& p) {
		ErrorShow(p);
		return -20;			// No Columns
	}

	ListCtrl->Freeze();

	// Make Columns ------------------
	ListCtrl->ClearAll();		// List Clear
	for (int i = 0; i < numCols; i++) {
		wxListItem itemCol1;
		itemCol1.SetText(colInf[i].name);
		itemCol1.SetAlign(wxLIST_FORMAT_LEFT);
		ListCtrl->InsertColumn(i, itemCol1);
		//int  dbtype is database dependent. Column data type code. For ODBC VARCHAR maybe 12 ?
			//DB_DATA_TYPE_VARCHAR)
		int t = colInf[i].dbtype;
		if (t < 0)
			t = otl_var_char;
		ListCtrl->SetColumnWidth(i, (colInf[i].dbtype == otl_var_char) ? 190 : 50);
	}


	// Fill Rows -------------------
	long		CurrRow = 0;
	CHAR_FROMDB	Field[1001];
	wxString	Translation;
	while (!sIn.eof()) { // while not end-of-data	2910ms	400ms Query, 2850 inserimento
		for (int i = 0; i < numCols; i++) {
			sIn >> Field;
			Translation = ((i == Fld2Translate) ? _(Field) : Field);
			if (i == 0)
				ListCtrl->InsertItem(CurrRow, Translation);
			else
				ListCtrl->SetItem(CurrRow, i, wxString::FromUTF8(Translation));
		}
		CurrRow++;
	}

	if (DoResize == true) {
		for (int i = 0; i < numCols; i++)
			ListCtrl->SetColumnWidth(i, wxLIST_AUTOSIZE);	//
	}

	ListCtrl->Thaw();
	return 0;
}

int Next_DB::ProgDetail_Fill(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize, int Fld2Translate) {
	return ListCtrl_FillFromSql(ListCtrl, wxString::Format(
		// Same order of cDetailListCtrl::PrgDetail_FillListItem
		//		StepId                      	  CmdTyp  SubCmd  [0]   [1]   [2]   [3]   ProgramId I2CAddr
//		"SELECT DetailProg AS[% s], Motor, Cmd AS[%s], Pattern, Cnt AS[%s], Par1, Par2, Par3, Par4, Par5, Par6, Par7 "
		"SELECT DetailProg AS[% s], Motor, Cmd AS[%s], CAST(Pattern AS VARCHAR(5)) AS Pattern, Cnt AS[%s], Par1, Par2, Par3, Par4, Par5, Par6, Par7 "
//		"SELECT DetailProg AS [%s], Name AS [%s], CmdTyp AS Board, SubCmd, Par1, Par2, Par3, Par4, MasterId, I2CAdd AS [%s] "
		" FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d ORDER BY DetailProg"
		, _("N")
		, _("Command")
		, _("Address")
		, ProgId
	)
		, DoResize, Fld2Translate);
}

long Next_DB::ProgMaster_GetNextId(unsigned int StartIdx) {
	wxString SqlCmd = wxString::Format(
		" SELECT MIN(ProgId) :#1<int> AS NextId"
		" FROM ("
		"	SELECT ProgId"
		" 	FROM " PROGMASTER_TABLENAME ""
		" 	UNION SELECT COUNT(*)+%d FROM " PROGMASTER_TABLENAME " WHERE 1=0"
		" ) AS A"
		" WHERE NOT EXISTS (SELECT 1 FROM " PROGMASTER_TABLENAME " B WHERE B.ProgId = A.ProgId+1)"
		" AND A.ProgId>=%d"
		, StartIdx, StartIdx
	);

	long LastId = 0;
	try {
		DBIterator o(1, SqlCmd.c_str(), AriesConn);
		if (!o.eof()) {
			o >> LastId;
			LastId++;
		}
	} catch (otl_exception& p) {
		ErrorShow(p);
	}
	return LastId;
}

bool Next_DB::ExecuteSQL(const wxString& SqlCmd, bool AutoCommit, bool IgnoreError) {
#ifdef _DEBUG
	IgnoreError = false;
#endif
	if (SqlCmd == "")	return true;
	try {
		AriesConn << SqlCmd.c_str();	//must be OTL_UNICODE_CHAR_TYPE
		if (AutoCommit == true)	AriesConn.commit();
		return true;
	}
	catch (otl_exception& p) {
		if (IgnoreError)
			return true;
		ErrorShow(p);
	}
	return false;
}

bool Next_DB::ProgMaster_Insert(const wxString& NewProgNameIn, unsigned int Id) {
	wxString NewProgName = SQLStrPrepare(NewProgNameIn).Left(50);
	if (Id == 0) {
		if ((Id = ProgMaster_GetNextId(NewProgNameIn.Left(4).IsSameAs("SUB_", false) ? 2000 : 0)) <= 0)	return false;
	}
	wxString SqlTest = wxString::Format("SELECT 1 FROM "	PROGMASTER_TABLENAME " WHERE ProgId = %d", Id);
	wxString SqlIns = wxString::Format("INSERT INTO "		PROGMASTER_TABLENAME " (ProgName, ProgId) VALUES ('%s', %d)", NewProgName, Id);
	wxString SqlUpd = wxString::Format("UPDATE "			PROGMASTER_TABLENAME " SET ProgName = '%s' WHERE ProgId = %d", NewProgName, Id);

	DBIterator i(1, SqlTest, AriesConn);
	return ExecuteSQL((i.eof()) ? SqlIns : SqlUpd, true);
}

bool Next_DB::ProgMaster_Copy(unsigned int ProgIdOld, const wxString& NewProgName, unsigned int ProgIdNew) {
	if (ProgIdNew == 0) {
		if ((ProgIdNew = ProgMaster_GetNextId(NewProgName.Left(4).IsSameAs("SUB_", false) ? 2000 : 0)) <= 0)
			return false;
	}
	if (ProgMaster_Insert(NewProgName, ProgIdNew)) {
		//3) Copy data
		wxString SqlIns = wxString::Format("INSERT INTO " PROGDETAIL_TABLENAME
			" (Name, I2CAdd, CmdTyp, Cmd, Par1, Par2, Par3, Par4, MasterId, DetailProg, SubCmd)"
			" SELECT Name, I2CAdd, CmdTyp, Cmd, Par1, Par2, Par3, Par4, %d, DetailProg, SubCmd"
			" FROM " PROGDETAIL_TABLENAME
			" WHERE MasterId = %d", ProgIdNew, ProgIdOld)
			;

		return ExecuteSQL(SqlIns, true);
	}
	return false;
}

bool Next_DB::ProgDetail_Delete(unsigned int ProgId, bool DelFather) {
	wxString Sql_Del = wxString::Format("DELETE FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d", ProgId);
	if (ExecuteSQL(Sql_Del, true) == true) {
		if (DelFather) {
			Sql_Del = wxString::Format("DELETE FROM " PROGMASTER_TABLENAME " WHERE ProgId = %d", ProgId);
			return ExecuteSQL(Sql_Del, true);
		}
	}
	return false;
}

DBIterator* Next_DB::ProgDetail_Open(unsigned int	ProgramId, unsigned int StepStart) {
	wxString	strSQL = wxString::Format("SELECT MasterId, DetailProg, Name, I2CAdd, CmdTyp, SubCmd, Par1, Par2, Par3, Par4"
		" FROM " PROGDETAIL_TABLENAME
		" WHERE MasterId = %d"
		"   AND DetailProg >= %d"
		" ORDER BY DetailProg"
		, ProgramId, StepStart
	);
	return GP_Open(strSQL);
}

bool Next_DB::ProgMaster_Print(unsigned int ProgId, const wxString& ProgName, const wxString& FilePathName) {
	DBIterator* Iter = ProgDetail_Open(ProgId);
	if (Iter) {
		wxFile file;
		if (!file.Open(FilePathName, wxFile::write))
			return false;	//Error (re)opening new file

		file.Write(wxString::Format("Process: %s:\n", ProgName));
		file.Write(wxString::Format("=========%s=\n", wxString('.', ProgName.Len())));

		//bool AppendBuffer = false;
/*		cNazarI2C V;
		while (PICProgram_Iterate(Iter, V, -1)) {	//Don't change sources data
			file.Write(wxString::Format("%s", V.Describe()));

			if (IppendBuffer) {
				// Append Buffer in hex
				file.Write("\t:");
				byte	bufCmd[20];
				byte	bufLen = V.ToBuff(bufCmd);
				for (size_t i = 0; i < bufLen; i++)
					file.Write(wxString::Format("%02X ", (unsigned int)bufCmd[i]));
				//---------------------------------
			}


			file.Write("\n");
		}
*/
		file.Close();
		ProgDetail_Close(Iter);
	}
	return true;
}


bool Next_DB::ProgMaster_Export(unsigned int ProgId, const wxString& FilePathName) {
#ifdef QQQQQQQQQQQQQQ
	tinyxml2::XMLDocument doc(true, tinyxml2::COLLAPSE_WHITESPACE);			//	TiXmlDocument doc;
	tinyxml2::XMLDeclaration* decl = doc.NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\"");	//	TiXmlDeclaration * decl	= new TiXmlDeclaration(	"1.0", "", "" );
	doc.InsertFirstChild(decl);																		//	doc.LinkEndChild( decl );	//Declaration
#endif
	return false;
}