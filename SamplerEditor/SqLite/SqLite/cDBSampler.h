#pragma once
#include "wx/wx.h"
#include "wx/listctrl.h"	//ListCtrl_FillFromSql
//#include <wx/dataview.h>	//wxDataViewListCtrl, wxmsw31??_adv.lib
#include "sqlite3.h" // Assicurati che questo file sia nel tuo percorso di inclusione
#include "cCmdStepper.h"

#define PROGMASTER_TABLENAME "SAM_ProgMaster"
#define PROGDETAIL_TABLENAME "SAM_ProgDetail"
/*
A) Funzioni che non devono conoscere i nomi di tabelle o campi
B) Funzioni a cui basta conoscere il nome della tabella
C) Funzioni che devono conoscere la struttura delle tabelle
*/

class cDBSampler {
	sqlite3* m_db = nullptr; // Database Pointer
	static void			ErrorShow(const char* zErrMsg);
	void				ListCtrl_FillFromSql(wxListCtrl* listCtrl, const wxString& SqlQuery, bool DoResize = true, int Fld2Translate = -1);
	long				ProgMaster_GetNextId(unsigned int StartIdx);
	static wxString		SQLStrPrepare(wxString str);
	bool				RecordExists(const wxString& Query);
	bool				ExecuteSQL(const wxString& queryToExecute, bool AutoCommit=true);
	bool				CreateTable(const char* sql_create);
	bool				CreateMaster(void);
	bool				CreateSlave(void);
public:
	cDBSampler(const char* filename = "../Sampler.db");
	~cDBSampler() {
		sqlite3_close(m_db);
	};
	const char* GetLastError(void) { return sqlite3_errmsg(m_db); }
	bool	CreateDB				(void);
	bool	ProgMaster_Insert		(const wxString& ProgName, unsigned int Id = 0);
	bool	ProgMaster_Copy			(unsigned int ProgIdOld, const wxString& NewProgName, unsigned int ProgIdNew = 0);
	void	ProgMaster_Fill2		(wxListCtrl* ListCtrl, bool SortByName, bool DoResize = true, int Fld2Translate = 1, byte Filter = 0);
	bool	ProgMaster_Export2		(unsigned int ProgId, const wxString& FilePathName);
	bool	ProgMaster_Print		(int ProgId, const wxString& ProgName, const wxString& filePath);

	bool	ProgDetail_Insert		(const cCmdStepper& Cmd, bool AllowRenum = true);
	bool	ProgDetail_Renum		(unsigned int ProgId);
	void	ProgDetail_Fill			(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize = true, int Fld2Translate = 1);
	bool	ProgDetail_Swap			(unsigned int ProgId, unsigned int iFrom, bool WithNext);
	bool	ProgDetail_Delete		(unsigned int ProgId, bool DelFather);
	bool	ProgDetail_Delete2		(unsigned int ProgId, unsigned int DetailId);
	//--------------------------------------------------
	void	DBCreateNewProcess(void);
	void	DBModifyCopyProcess(const wxString& Name, const unsigned int ProgId, bool Modify = true);

};