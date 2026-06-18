#pragma once
#include "wx/wx.h"
#include "wx/listctrl.h"	//ListCtrl_FillFromSql
#include <wx/dataview.h>	//wxDataViewListCtrl, wxmsw31??_adv.lib
#include "sqlite3.h"
#include <functional>

#define PROGMASTER_TABLENAME "SAM_ProgMaster"
#define PROGDETAIL_TABLENAME "SAM_ProgDetail"
//--------------------------------------------------
#include <wx/ffile.h>
#include <tinyxml2.h>
#include <memory>

struct ExportStepData {
	sCommand		C;
	wxString		subSysName;
	wxString		commandName;
	wxArrayString	ParamsList;
};

class IExportWriter {
public:
	virtual ~IExportWriter() = default;
	virtual bool Open(const wxString& filePath) = 0;
	virtual void WriteMasterField(const char* name, const char* value) = 0;
	virtual void BeginDetails() = 0;
	virtual void WriteDetailStep(const ExportStepData& step) = 0;
	virtual bool Close() = 0;
};

class TextExportWriter : public IExportWriter {
	wxFFile m_file;
public:
	bool Open(const wxString& filePath) override {
		if (!m_file.Open(filePath, "w")) return false;
		m_file.Write(wxString::Format("=== MASTER PROGRAMM ===\n"));
		return true;
	}
	void WriteMasterField(const char* name, const char* value) override {
		m_file.Write(wxString::Format("%s: %s\n", name, value));
	}
	void BeginDetails() override {
		m_file.Write("\n--- DETAILS ---\n");
		m_file.Write("> DetailId; SubSys; Command; PatLen; Pattern; Par0; Par1; Par2; Par3; Par4; Par5; Par6; Par7; Par8; Par9;\n");

	}
	void WriteDetailStep(const ExportStepData& s) override {
		wxString line = "> ";
		line += wxString::Format("\"%03ld\"; ",	s.C.m_DetailProg);
		line += wxString::Format("\"%s\"; ",	s.subSysName);
		line += wxString::Format("\"%s\"; ",	s.commandName);
		line += wxString::Format("\"%d\"; ",	s.C.m_PatLen);
		line += wxString::Format("\"%s\"; ",	(char*)s.C.m_Pattern);
		for (size_t i = 0; i < s.C.m_PatLen; ++i) {
			line += wxString::Format("\"%ld\"; ", s.C.m_Par[i]);
		}
		m_file.Write(line + "\n");
	}
	bool Close() override { m_file.Close(); return true; }
};

class XmlExportWriter : public IExportWriter {
	tinyxml2::XMLDocument m_xml;
	tinyxml2::XMLElement* m_masterElement = nullptr;
	tinyxml2::XMLElement* m_detailsElement = nullptr;
	wxString m_filePath;
public:
	bool Open(const wxString& filePath) override {
		m_filePath = filePath;
		auto* decl = m_xml.NewDeclaration();
		m_xml.InsertFirstChild(decl);
		auto* root = m_xml.NewElement("ProgramExport");
		m_xml.InsertEndChild(root);
		m_masterElement = m_xml.NewElement("Master");
		root->InsertEndChild(m_masterElement);
		return true;
	}
	void WriteMasterField(const char* name, const char* value) override {
		if (m_masterElement) m_masterElement->SetAttribute(name, value);
	}
	void BeginDetails() override {
		if (m_masterElement) {
			m_detailsElement = m_xml.NewElement("Details");
			m_masterElement->InsertEndChild(m_detailsElement);
		}
	}
	void WriteDetailStep(const ExportStepData& s) override {
		if (!m_detailsElement) return;
		auto* stepElement = m_xml.NewElement("Step");
		m_detailsElement->InsertEndChild(stepElement);
		stepElement->SetAttribute("DetailProg",	(int64_t)s.C.m_DetailProg);
		stepElement->SetAttribute("SubSys",		s.subSysName.utf8_str());
		stepElement->SetAttribute("Cmd",		s.commandName.utf8_str());
		stepElement->SetAttribute("PatLen",		s.C.m_PatLen);
		stepElement->SetAttribute("Pattern",	(char*)s.C.m_Pattern);
		for (size_t i = 0; i < s.C.m_PatLen; ++i) {
			stepElement->SetAttribute(wxString::Format("Par%zu", i).utf8_str(), s.ParamsList.Item(i).utf8_str());
		}
	}
	bool Close() override {
		return m_xml.SaveFile(m_filePath.utf8_str()) == tinyxml2::XML_SUCCESS;
	}
};


class ExportWriterFactory {
public:
	static std::unique_ptr<IExportWriter> CreateWriter(bool isText) {
		if (isText) {
			return std::make_unique<TextExportWriter>();
		} else {
			return std::make_unique<XmlExportWriter>();
		}
	}
};
//-------------------------------------------------------------
class cDBSampler {
	sqlite3* m_db = nullptr; // Database Pointer
	static void			ErrorShow(const char* zErrMsg);
	void				ListCtrl_FillFromSql(wxListCtrl* listCtrl, const wxString& SqlQuery, bool DoResize = true, int Fld2Translate = -1);

	long				ProgMaster_GetNextId(unsigned int StartIdx);
	static wxString		SQLStrPrepare(const wxString& str);
	bool				RecordExists(const wxString& Query);
	bool				ExecuteSQL(const wxString& queryToExecute, bool AutoCommit=true);
	bool				CreateTable(const char* sql_create);
	bool				CreateMaster(void);
	bool				CreateSlave(void);
	static wxString		SqlQuery_Detail(const unsigned int ProgId);
public:
	cDBSampler(const char* filename = "../Sampler.db");
	~cDBSampler() { if (m_db) sqlite3_close(m_db); };

	cDBSampler(const cDBSampler&) = delete;
	cDBSampler& operator=(const cDBSampler&) = delete;

	const char* GetLastError(void) { return sqlite3_errmsg(m_db); }
	wxString	getMasterName(unsigned int ProgId);

	bool	CreateDB				(void);
	bool	ProgMaster_Insert		(const wxString& ProgName, unsigned int Id = 0);
	bool	ProgMaster_Copy			(unsigned int ProgIdOld, const wxString& NewProgName, unsigned int ProgIdNew = 0);
	void	ProgMaster_Fill2		(wxListCtrl* ListCtrl, bool SortByName, bool DoResize = true, int Fld2Translate = 1, byte Filter = 0);
	bool	ProgMaster_Export		(bool IsText, unsigned int ProgId, const wxString& FilePathName);

	bool	ProgDetail_Insert		(const sCommand& Cmd, bool AllowRenum = true);
	bool	ProgDetail_Renum		(unsigned int ProgId, unsigned int Step = 3);
	void	ProgDetail_Fill			(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize = true, int Fld2Translate = 1);
	bool	ProgDetail_Swap			(unsigned int ProgId, unsigned int iFrom, bool WithNext);
	bool	ProgDetail_Delete		(unsigned int ProgId, bool DelFather);
	bool	ProgDetail_Delete2		(unsigned int ProgId, unsigned int DetailId);
	//--------------------------------------------------
	void	ProgDetail_ReadAll		(unsigned int ProgId, std::function<void(const wxString&, bool)> logFunc);

	void	DBCreateNewProcess(void);
	void	DBModifyCopyProcess(const wxString& Name, const unsigned int ProgId, bool Modify = true);

	void	DataViewCtrl_FillFromSql(wxDataViewListCtrl* dvCtrl, const wxString& SqlQuery, bool DoResize);

	bool	ProgDetail_Select(int64_t masterId, int64_t detailProg, sCommand& item);

};