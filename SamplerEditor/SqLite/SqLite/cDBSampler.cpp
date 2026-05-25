#include "stdwx.h"
#include "cDBSampler.h"

#include <iostream>
#include <string>
#include <wx/ffile.h> // Piu' efficiente per grandi volumi di dati

#define USEXML
//#define USEIMPORT
#if defined(USEXML)
#include "tinyxml2.h"   //https://github.com/leethomason/tinyxml2
using namespace tinyxml2;
#endif

static int callback(void* , int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << "---------------------------------" << std::endl;
    return 0;
}

void cDBSampler::ErrorShow(const char* zErrMsg) {
    wxMessageBox(zErrMsg, "SQL Error!", wxOK | wxICON_INFORMATION, NULL);
    //	wxMessageDialog(NULL,msg,"SQL Error!", wxOK | wxICON_INFORMATION );
}

cDBSampler::cDBSampler(const char* filename) {
    //Check if exists
    //int rc = sqlite3_open_v2(filename, &m_db, SQLITE_OPEN_READWRITE, NULL); //Error if it doesn't exist
    int rc = sqlite3_open(filename, &m_db);                               //Create it if it doesn't exist
    if (rc) {
        ErrorShow(sqlite3_errmsg(m_db));
        return;
    }
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
};

long cDBSampler::ProgMaster_GetNextId(unsigned int StartIdx) {
    if (!m_db) return -1; // O un valore di errore appropriato
    sqlite3_stmt* stmt;
    long nextId = StartIdx; // Default per la tabella vuota o query fallta
    wxString SqlCmd = wxString::Format(
        "SELECT MIN(ProgId) + 1 AS NextId "
        "FROM ("
        "  SELECT ProgId FROM %s "
        "  UNION SELECT %u " // Semplificato
        ") AS A "
        "WHERE NOT EXISTS (SELECT 1 FROM %s B WHERE B.ProgId = A.ProgId + 1) "
        "AND A.ProgId >= %u",
        PROGMASTER_TABLENAME, StartIdx, PROGMASTER_TABLENAME, StartIdx
    );

    // 1. Preparazione
    if (sqlite3_prepare_v2(m_db, SqlCmd.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return -1;
    }

    // 2. Esecuzione
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Estraiamo il valore dalla colonna 0 (MIN(ProgId) + 1)
        // Usiamo sqlite3_column_int64 per gestire ID potenzialmente grandi (long)
        nextId = (long)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);

    // Se 0 come risultato (per la tabella vuota e StartIdx era 0), restituiamo almeno StartIdx
    return (nextId < (long)StartIdx) ? (long)StartIdx : nextId;
}

void cDBSampler::ListCtrl_FillFromSql(wxListCtrl* listCtrl, const wxString& SqlQuery, bool DoResize, int /*Fld2Translate*/ ) {
    if (!m_db || !listCtrl) return;

    sqlite3_stmt* stmt;

    // Preparazione della query
    if (sqlite3_prepare_v2(m_db, SqlQuery.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return;
    }
    int colCount = sqlite3_column_count(stmt);

    listCtrl->Freeze();
    listCtrl->ClearAll();   // Svuota la lista esistente

    // 1. Creazione delle intestazioni e dimensionamento colonne
    for (int i = 0; i < colCount; i++) {
        wxString colName = wxString::FromUTF8(sqlite3_column_name(stmt, i));
        //int colType = sqlite3_column_type(stmt, i); // Nota: il tipo e' affidabile solo dopo uno step o tramite decltype

        // SQLite e' dinamico, ma possiamo tentare di dedurre il tipo dalla definizione
        const char* declType = sqlite3_column_decltype(stmt, i);
        wxString typeStr = (declType) ? wxString(declType).Upper() : "";

        // Dimensionamento: 50 per numerici (INT, REAL, DOUBLE, FLOAT), 190 per altri
        int width = (   typeStr.Contains("INT") ||
                        typeStr.Contains("REAL") ||
                        typeStr.Contains("DOUBLE") ||
                        typeStr.Contains("FLOAT") ||
                        typeStr.Contains("NUMERIC")
                    ) ? 50 : 190;
        listCtrl->InsertColumn(i, colName, wxLIST_FORMAT_LEFT, width);
    }

    // 2. Inserimento dei dati
    int rowIndex = 0;
    //wxString	Translation;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        long itemIndex = listCtrl->InsertItem(rowIndex, "");
        for (int col = 0; col < colCount; col++) {
            const char* val = (const char*)sqlite3_column_text(stmt, col);
           // Translation = ((col == Fld2Translate) ? _(val) : val);
            wxString cellValue = (val) ? wxString::FromUTF8(val) : "";
            /*if (col > 6) {
                //listCtrl->SetItemBackgroundColour(itemIndex, *wxLIGHT_GREY);
                wxListItem item;
                item.SetId(itemIndex);
                item.SetColumn(col); // Specifica la colonna
                item.SetBackgroundColour(*wxYELLOW);
                listCtrl->SetItem(item);
            }*/
            listCtrl->SetItem(itemIndex, col, cellValue);
        }
        rowIndex++;
    }
    sqlite3_finalize(stmt);

    if (DoResize == true) {
        for (int i = 0; i < colCount; i++)
            listCtrl->SetColumnWidth(i, wxLIST_AUTOSIZE);	//
    }

    listCtrl->Thaw();
}

//In Progress!!!!!
void cDBSampler::DataViewCtrl_FillFromSql(wxDataViewListCtrl* dvCtrl, const wxString& SqlQuery, bool DoResize) {
    if (!m_db || !dvCtrl) return;

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, SqlQuery.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return;
    }

    int colCount = sqlite3_column_count(stmt);
    dvCtrl->Freeze();

    // Rimuove tutte le colonne e gli elementi esistenti
    dvCtrl->ClearColumns();
    dvCtrl->DeleteAllItems();

    // 1. Creazione delle Colonne
    for (int i = 0; i < colCount; i++) {
        wxString colName = wxString::FromUTF8(sqlite3_column_name(stmt, i));
        const char* declType = sqlite3_column_decltype(stmt, i);
        wxString typeStr = (declType) ? wxString(declType).Upper() : "";

        // Calcolo larghezza
        int width = (typeStr.Contains("INT") || typeStr.Contains("REAL") ||
            typeStr.Contains("DOUBLE") || typeStr.Contains("FLOAT") ||
            typeStr.Contains("NUMERIC")) ? 60 : 200;

        // In wxDataViewCtrl aggiungiamo renderer di testo
        dvCtrl->AppendTextColumn(colName, wxDATAVIEW_CELL_INERT, width, wxALIGN_LEFT);
    }

    // 2. Inserimento dei Dati
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        wxVector<wxVariant> data; // Contenitore per la riga
        for (int col = 0; col < colCount; col++) {
            const char* val = (const char*)sqlite3_column_text(stmt, col);
            wxString cellValue = (val) ? wxString::FromUTF8(val) : "";
            data.push_back(wxVariant(cellValue));
        }
        dvCtrl->AppendItem(data);
    }

    sqlite3_finalize(stmt);

    // 3. Resize (Nota: wxDataViewCtrl non ha AUTOSIZE identico a ListCtrl)
    if (DoResize) {
        for (unsigned int i = 0; i < dvCtrl->GetColumnCount(); i++) {
            // Su molte piattaforme bisogna impostare una larghezza fissa o variabile
            dvCtrl->GetColumn(i)->SetWidth(wxCOL_WIDTH_AUTOSIZE);
        }
    }

    dvCtrl->Thaw();
}


void cDBSampler::ProgMaster_Fill2(wxListCtrl* ListCtrl, bool SortByName, bool DoResize, int Fld2Translate, byte Filter) {
    wxString SqlWhere;
    switch (Filter) {
        case 1:	SqlWhere << "WHERE ProgId < 2000 ";		break;	//User
        case 2:	SqlWhere << "WHERE ProgId < 2000 ";		break;	//Service
        case 3:	SqlWhere << "WHERE ProgId >=2000 ";		break;	//Subroutines
        case 0:	//All
        default:										break;
    }

    wxString	SqlCmd = wxString::Format("SELECT ProgId AS [%s], ProgName AS [%s] FROM " PROGMASTER_TABLENAME " %s ORDER BY %s "
        , _("Program Id")
        , _("Program Name")
        , SqlWhere
        , (SortByName ? "2" : "1")
    );
    ListCtrl_FillFromSql(ListCtrl, SqlCmd, DoResize, Fld2Translate);
}

void cDBSampler::ProgDetail_ReadAll(unsigned int ProgId, std::function<void(const wxString&, bool)> logFunc) {
    wxString SqlQuery = SqlQuery_Detail(ProgId);
    if (!m_db) return;
    sqlite3_stmt* stmt;

    // Preparazione della query
    if (sqlite3_prepare_v2(m_db, SqlQuery.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return;
    }
    int colCount = sqlite3_column_count(stmt);
    void			LogMe(const wxString & ToLog, bool PrependTime);
    // 1. Creazione delle intestazioni e dimensionamento colonne
    for (int i = 0; i < colCount; i++) {
        wxString colName = wxString::FromUTF8(sqlite3_column_name(stmt, i));
        logFunc(colName, false);
        logFunc(" | ", false);
    }
    logFunc("\n", false);

    // 2. Inserimento dei dati
    int rowIndex = 0;
    wxString	Translation;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int col = 0; col < colCount; col++) {
            const char* val = (const char*)sqlite3_column_text(stmt, col);
            wxString cellValue = (val) ? wxString::FromUTF8(val) : "";
            logFunc(cellValue, false);
            logFunc(" | ", false);
        }
        logFunc("\n", false);
        rowIndex++;
    }
    sqlite3_finalize(stmt);
}


wxString cDBSampler::SqlQuery_Detail(const unsigned int ProgId) {
    wxString SqlQuery = "SELECT ";
    for (int i = 0; i < efielsCount; i++) {
        switch ((eDetHeaders)i) {
            case eDetailProg:   SqlQuery += "DetailProg AS [N], ";      break;
            case eSubSys:       SqlQuery += "SubSys, ";                 break;
            case eCmd:          SqlQuery += "Cmd, ";                    break;
            case ePattern:      SqlQuery += "Pattern AS [Command], ";   break;
            case eMasterId:                                             break;    //Will be last
            default:
                if (i < eParFirst || i > eParLast)
                    wxLogError(wxT("Warning: Review eDetHeaders list!"));
                else
                    SqlQuery += wxString::Format("Par%d, ", i-eParFirst);
                break;
        }
    }
    SqlQuery += wxString::Format("MasterId FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d ORDER BY DetailProg", ProgId);
    return SqlQuery;
}


void cDBSampler::ProgDetail_Fill(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize, int Fld2Translate) {
    ListCtrl_FillFromSql(ListCtrl, SqlQuery_Detail(ProgId), DoResize, Fld2Translate);
}

wxString cDBSampler::SQLStrPrepare(wxString str) {
    wxString	SqlStr = str;
    SqlStr.Replace("'", "''", true);
    return SqlStr;
}

bool cDBSampler::RecordExists(const wxString& Query) {
    if (!m_db) return false;
    sqlite3_stmt* stmt;
    bool exists = false;

    int rc = sqlite3_prepare_v2(m_db, Query.utf8_str(), -1, &stmt, nullptr);    // 1. Preparazione della query (converte wxString in UTF-8)
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {     // 2. Esecuzione del primo step: se restituisce SQLITE_ROW, il record esiste
            exists = true;
        }
    } else {
        ErrorShow(sqlite3_errmsg(m_db));
    }
    sqlite3_finalize(stmt);         // 3. Finalizzazione obbligatoria per liberare le risorse dello statement
    return exists;
}

bool cDBSampler::ExecuteSQL(const wxString& queryToExecute, bool AutoCommit) {
    if (!m_db) return false;
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(m_db, queryToExecute.utf8_str(), nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        ErrorShow(zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    if(AutoCommit)  sqlite3_exec(m_db, "COMMIT;", NULL, NULL, NULL);
    return true;
}

bool cDBSampler::ProgMaster_Insert(const wxString& ProgName, unsigned int Id) {
    wxString NewProgName = SQLStrPrepare(ProgName).Left(50);
    if (Id == 0) {
       if ((Id = ProgMaster_GetNextId(NewProgName.Left(4).IsSameAs("SUB_", false) ? 2000 : 0)) <= 0)	return false;
    }
    wxString SqlTest = wxString::Format("SELECT 1 FROM "	PROGMASTER_TABLENAME " WHERE ProgId = %d", Id);
    wxString SqlIns = wxString::Format("INSERT INTO "		PROGMASTER_TABLENAME " (ProgName, ProgId) VALUES ('%s', %d)", NewProgName, Id);
    wxString SqlUpd = wxString::Format("UPDATE "			PROGMASTER_TABLENAME " SET ProgName = '%s' WHERE ProgId = %d", NewProgName, Id);
    return ExecuteSQL(RecordExists(SqlTest) ? SqlUpd : SqlIns);
}

//========================================================
void cDBSampler::DBCreateNewProcess(void) {
    wxTextEntryDialog dialog(nullptr,
        _("Insert a name for the new process"),
        _("Insert a new Process"),
        _("New Process"),
        wxOK | wxCANCEL
    );

    if (dialog.ShowModal() == wxID_OK && dialog.GetValue() != wxT("")) {
        ProgMaster_Insert(dialog.GetValue());
        //MainPrg_Fill();
    }
}

bool cDBSampler::ProgMaster_Copy(unsigned int ProgIdOld, const wxString& NewProgName, unsigned int ProgIdNew) {
    if (ProgIdNew == 0) {
        if ((ProgIdNew = ProgMaster_GetNextId(NewProgName.Left(4).IsSameAs("SUB_", false) ? 2000 : 0)) <= 0)
            return false;
    }
    if (ProgMaster_Insert(NewProgName, ProgIdNew)) {
        //3) Copia i dati dalla vecchia sequenza

        wxString strParNames;
        for (byte i = 0; i < NUMOFPARAMS; i++) {
            strParNames += wxString::Format(", Par%d", i);
        }

        wxString SqlCmd = wxString::Format(
            "INSERT INTO " PROGDETAIL_TABLENAME 
                           " (MasterId, DetailProg, SubSys, Cmd, Pattern %s ) "
                            "SELECT %d, DetailProg, SubSys, Cmd, Pattern %s "
            "FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d;",
            strParNames, ProgIdNew,
            strParNames, ProgIdOld
        );

        return ExecuteSQL(SqlCmd, true);
    }
    return false;
}

bool cDBSampler::CreateTable(const char* sql_create) {
    char* zErrMsg = 0;
    int rc = sqlite3_exec(m_db, sql_create, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        ErrorShow(zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool cDBSampler::CreateSlave(void) {
    wxString strParDefs;
    for (byte i = 0; i < NUMOFPARAMS; i++) {
        strParDefs += wxString::Format("Par%d          INTEGER,", i);
    }

    wxString sql_create = wxString::Format(
        "CREATE TABLE IF NOT EXISTS " PROGDETAIL_TABLENAME " ("
            "MasterId      INTEGER NOT NULL,"
            "DetailProg    INTEGER NOT NULL,"
            "SubSys        INTEGER NOT NULL,"
            "Cmd           INTEGER NOT NULL,"
            "Pattern       TEXT NOT NULL DEFAULT '',"
            "%s"
            "PRIMARY KEY (MasterId, DetailProg),"
            "FOREIGN KEY (MasterId) REFERENCES " PROGMASTER_TABLENAME " (ProgId) "
            "ON DELETE CASCADE ON UPDATE CASCADE"
        ") WITHOUT ROWID;" // Optimization for table with PK compound
        , strParDefs
    );

    return CreateTable(sql_create.utf8_str());
}

bool cDBSampler::CreateMaster(void) {
    const char* sql_create = "CREATE TABLE IF NOT EXISTS " PROGMASTER_TABLENAME "("
                                "ProgId INT PRIMARY KEY NOT NULL,"
                                "ProgName   TEXT NOT NULL);";
    return CreateTable(sql_create);
}

bool cDBSampler::CreateDB(void) {
    CreateMaster();
    CreateSlave();
    if (!ProgMaster_Insert("BravoBaldo")) return false;

    return true;
}

wxString cDBSampler::getMasterName(unsigned int ProgId) {
    if (!m_db) return wxEmptyString;
    sqlite3_stmt* stmtMaster = nullptr;
    const char* query = "SELECT ProgName FROM " PROGMASTER_TABLENAME " WHERE ProgId = ?;";
    if (sqlite3_prepare_v2(m_db, query, -1, &stmtMaster, nullptr) != SQLITE_OK) return wxEmptyString;
    sqlite3_bind_int(stmtMaster, 1, static_cast<int>(ProgId));

    wxString result;
    if (sqlite3_step(stmtMaster) == SQLITE_ROW) {
        if (const unsigned char* text = sqlite3_column_text(stmtMaster, 0)) {
            result = wxString::FromUTF8(reinterpret_cast<const char*>(text));
        }
    }
    sqlite3_finalize(stmtMaster);
    return result;
}

bool cDBSampler::ProgMaster_Export(bool IsText, unsigned int ProgId, const wxString& FilePathName) {
    if (!m_db) return false;

    auto writer = ExportWriterFactory::CreateWriter(IsText);    //Ask handle to Factory

    // 2. Lettura e scrittura Master
    sqlite3_stmt* stmtMaster = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT * FROM " PROGMASTER_TABLENAME " WHERE ProgId = ?;", -1, &stmtMaster, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmtMaster, 1, ProgId);

    if (sqlite3_step(stmtMaster) != SQLITE_ROW) {
        sqlite3_finalize(stmtMaster);
        return false;
    }

    if (!writer->Open(FilePathName)) {
        sqlite3_finalize(stmtMaster);
        return false;
    }

    int cols = sqlite3_column_count(stmtMaster);
    for (int i = 0; i < cols; i++) {
        const char* colName = sqlite3_column_name(stmtMaster, i);
        const char* colVal = (const char*)sqlite3_column_text(stmtMaster, i);
        writer->WriteMasterField(colName, colVal ? colVal : "");
    }
    sqlite3_finalize(stmtMaster);

    // 3. Predisposizione sezione dettagli
    writer->BeginDetails();

    // 4. Lettura e scrittura Slave
    sqlite3_stmt* stmtSlave = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT * FROM " PROGDETAIL_TABLENAME " WHERE MasterId = ? ORDER BY DetailProg;", -1, &stmtSlave, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmtSlave, 1, ProgId);
        int sCols = sqlite3_column_count(stmtSlave);

        while (sqlite3_step(stmtSlave) == SQLITE_ROW) {
            sCommand S; // Tua struct originale per il parsing
            for (int j = 0; j < sCols; j++) {
                const char* sColVal = (const char*)sqlite3_column_text(stmtSlave, j);
                switch (j) {
                case 0: S.m_MasterId    = sColVal ? atol(sColVal) : 0; break;
                case 1: S.m_DetailProg  = sColVal ? atol(sColVal) : 0; break;
                case 2: S.m_SubSystem   = (eSubSysAcro)(sColVal ? atol(sColVal) : 0); break;
                case 3: S.m_Cmd         = sColVal ? atol(sColVal) : 0; break;
                case 4: S.m_PatLen      = sColVal ? strlen(sColVal) : 0;
                    for (byte i = 0; i < NUMOFPARAMS; ++i)
                        S.m_Pattern[i] = (i < S.m_PatLen) ? sColVal[i] : '\0';
                    break;
                default:
                    if (j >= 5 && j <= 14) S.m_Par[j - 5] = sColVal ? atol(sColVal) : 0;
                    break;
                }
            }
            const sSubSystem        *Ssys = SubSystem_GetByType(S.m_SubSystem);
            const sSampler_Commands *pCmd = Command_GetByCmd((char)S.m_SubSystem, (char)S.m_Cmd, S.m_PatLen);

            // We package the extracted data into a clean struct independent of the output format
            ExportStepData stepData;
            stepData.C = S;
            stepData.subSysName     = wxString::Format("%d-%s", (int)S.m_SubSystem, Ssys ? Ssys->Descr : "?");
            stepData.commandName    = wxString::Format("%d-%s", (int)S.m_Cmd,       pCmd ? pCmd->Descr : "?");

            //.............................
            stepData.ParamsList.Clear();
            for (size_t i = 0; i < S.m_PatLen; ++i) {
                wxString strVal = wxString::Format("%ld", S.m_Par[i]);
                if (S.m_SubSystem == eSystemCmd && S.m_Cmd == 'a' && S.m_Pattern[i] == 'P') {	//"Exec. Routine"
                    //strVal+= getMasterName(S.m_Par[i]);
                    strVal += wxString::Format("=*%s*", getMasterName(S.m_Par[i]));
                }
                const sParams* P = Param_Get(S.m_Pattern[i]);
                if (P && P->ParType == eChoice) {
                    strVal += "=" + P->ParValues[S.m_Par[i]];
                }
                stepData.ParamsList.Add(strVal);
            }
            //............................................
            writer->WriteDetailStep(stepData);  // Scrittura agnostica del formato
        }
        sqlite3_finalize(stmtSlave);
    }
    return writer->Close();     // 5. Chiusura e salvataggio del file
}



#if defined(USEXML)
#if defined(USEIMPORT)

/*
#include <sqlite3.h>
#include "tinyxml2.h"
#include <wx/string.h>
#include <vector>

using namespace tinyxml2;
*/
bool cDBSampler::ImportFromXML_Tiny(const wxString& filePath) {
    if (!m_db) return false;

    XMLDocument xml;
    if (xml.LoadFile(filePath.utf8_str()) != XML_SUCCESS) return false;

    XMLElement* root = xml.FirstChildElement("ProgramExport");
    if (!root) return false;

    XMLElement* masterElem = root->FirstChildElement("Master");
    if (!masterElem) return false;

    // 1. Inizio Transazione
    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    // 2. Preparazione inserimento Master
    // Nota: Usiamo una logica dinamica basata sugli attributi presenti nell'XML
    wxString masterCols, masterValues;
    const XMLAttribute* attr = masterElem->FirstAttribute();
    while (attr) {
        masterCols += wxString::Format("%s%s", masterCols.IsEmpty() ? "" : ", ", attr->Name());
        masterValues += wxString::Format("%s'%s'", masterValues.IsEmpty() ? "" : ", ", attr->Value());
        attr = attr->Next();
    }

    wxString sqlMaster = wxString::Format("INSERT INTO %s (%s) VALUES (%s);",
        PROGMASTER_TABLENAME, masterCols, masterValues);

    if (sqlite3_exec(m_db, sqlMaster.utf8_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    // Recuperiamo l'ID appena inserito (se il master usa un ID autoincrement o specifico)
    // Se l'ID e' esplicitamente nel file XML, lo useremo per gli slave.
    int newMasterId = masterElem->IntAttribute("ProgId");

    // 3. Importazione Slave (Details)
    XMLElement* detailsElem = masterElem->FirstChildElement("Details");
    if (detailsElem) {
        XMLElement* stepElem = detailsElem->FirstChildElement("Step");
        while (stepElem) {
            wxString slaveCols, slaveValues;
            const XMLAttribute* sAttr = stepElem->FirstAttribute();
            while (sAttr) {
                slaveCols += wxString::Format("%s%s", slaveCols.IsEmpty() ? "" : ", ", sAttr->Name());
                // Se l'attributo e' MasterId, assicuriamoci che corrisponda al nuovo master
                if (wxString(sAttr->Name()).CmpNoCase("MasterId") == 0) {
                    slaveValues += wxString::Format("%s%d", slaveValues.IsEmpty() ? "" : ", ", newMasterId);
                } else {
                    slaveValues += wxString::Format("%s'%s'", slaveValues.IsEmpty() ? "" : ", ", sAttr->Value());
                }
                sAttr = sAttr->Next();
            }

            wxString sqlSlave = wxString::Format("INSERT INTO %s (%s) VALUES (%s);",
                PROGDETAIL_TABLENAME, slaveCols, slaveValues);

            if (sqlite3_exec(m_db, sqlSlave.utf8_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
                sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
                return false;
            }
            stepElem = stepElem->NextSiblingElement("Step");
        }
    }

    // 4. Fine Transazione
    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}
#endif
#endif





#define TENTA3
bool cDBSampler::ProgDetail_Renum(unsigned int ProgId, unsigned int Step) {
#ifdef TENTA3
    if (!m_db) return false;
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) return false;

    wxString sqlTemp = wxString::Format(
        "WITH NewSequence AS ("
        "  SELECT MasterId, DetailProg, "
        "  (ROW_NUMBER() OVER (PARTITION BY MasterId ORDER BY DetailProg) * %u) AS NewVal "
        "  FROM SAM_ProgDetail "
        "  WHERE MasterId = %u"
        ") "
        "UPDATE SAM_ProgDetail "
        "SET DetailProg = -(SELECT NewVal FROM NewSequence "
        "                  WHERE NewSequence.MasterId = SAM_ProgDetail.MasterId "
        "                  AND NewSequence.DetailProg = SAM_ProgDetail.DetailProg) "
        "WHERE MasterId = %u;", Step, ProgId, ProgId
    );
    wxString sqlUpdate = wxString::Format(
        "UPDATE SAM_ProgDetail SET DetailProg = ABS(DetailProg) WHERE MasterId = %u;"// AND DetailProg < 0
        , ProgId
    );

    bool Res1 = sqlite3_exec(m_db, sqlTemp.utf8_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
    if (!Res1)    ErrorShow(sqlite3_errmsg(m_db));
    bool Res2 = sqlite3_exec(m_db, sqlUpdate.utf8_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
    if (!Res2)    ErrorShow(sqlite3_errmsg(m_db));
    // Esecuzione sequenziale
    if (Res1 && Res2 ) {
        sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
        return true;
    }
    else {
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
#endif
}


bool cDBSampler::ProgDetail_Insert(const sCommand& item, bool AllowRenum ) {
    if (!m_db) return false;

    wxString strParNames;
    wxString strParams;
    wxString strParExc;
    for (byte i = 0; i < NUMOFPARAMS; i++) {
        strParNames += wxString::Format(", Par%d", i);
        strParams += ", ?";
        strParExc += wxString::Format(", Par%d=excluded.Par%d", i, i);
    }
    wxString sql = wxString::Format(
        "INSERT INTO %s (MasterId, DetailProg, SubSys, Cmd, Pattern %s) VALUES (?, ?, ?, ?, ?%s) "
        "ON CONFLICT(MasterId, DetailProg) DO UPDATE SET "
        "SubSys=excluded.SubSys, Cmd=excluded.Cmd, Pattern=excluded.Pattern %s;",
        PROGDETAIL_TABLENAME, strParNames, strParams, strParExc
    );

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return false;
    }

    // Binding dei parametri dall'oggetto sCommand
    sqlite3_bind_int64(stmt, 1, item.m_MasterId);
    sqlite3_bind_int64(stmt, 2, item.m_DetailProg);
    sqlite3_bind_int  (stmt, 3, item.m_SubSystem);
    sqlite3_bind_int  (stmt, 4, item.m_Cmd);
    sqlite3_bind_text (stmt, 5, item.GetPatternAsChars(), -1, SQLITE_TRANSIENT);

    int i;
    for (i = 0; i < item.m_PatLen; i++) sqlite3_bind_int64(stmt, 6 + i, item.m_Par[i]);
    for (; i < NUMOFPARAMS; i++)        sqlite3_bind_int64(stmt, 6 + i, 0);

    sqlite3_step(stmt); //Execution

    if (sqlite3_finalize(stmt) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return false;
    }
    if (AllowRenum)
        ProgDetail_Renum(item.m_MasterId);    //Update don't require renumbering

    return true;
}

bool cDBSampler::ProgDetail_Swap(unsigned int ProgId, unsigned int iFrom, bool WithNext) {
    sqlite3_stmt* stmt;
    wxString sql = wxString::Format(
                                "SELECT DetailProg FROM %s WHERE MasterId = ? AND DetailProg %c ? ORDER BY DetailProg %s LIMIT 1;",
                                PROGDETAIL_TABLENAME, WithNext?'>':'<', WithNext ? "ASC":"DESC"
                            );
    if (sqlite3_prepare_v2(m_db, sql.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, ProgId);
    sqlite3_bind_int64(stmt, 2, iFrom);
    long	neighborId =-1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        neighborId = (long)sqlite3_column_int64(stmt, 0);
        wxString sqlCurrent  = wxString::Format("UPDATE " PROGDETAIL_TABLENAME " SET DetailProg = %d WHERE MasterId = %d AND DetailProg = %d", 99999, ProgId, iFrom);
        wxString sqlNext     = wxString::Format("UPDATE " PROGDETAIL_TABLENAME " SET DetailProg = %d WHERE MasterId = %d AND DetailProg = %d", iFrom, ProgId, neighborId);
        wxString sqlCurrent2 = wxString::Format("UPDATE " PROGDETAIL_TABLENAME " SET DetailProg = %d WHERE MasterId = %d AND DetailProg = %d", neighborId, ProgId, 99999);

        bool	Result = false;
        if ((Result = ExecuteSQL(sqlCurrent, false)) == true) {
            if ((Result = ExecuteSQL(sqlNext, false)) == true) {
                if ((Result = ExecuteSQL(sqlCurrent2, false)) == true) {
                    Result = true;
                }
            }
        }
        sqlite3_exec(m_db, (Result == true)?"COMMIT;":"ROLLBACK;", nullptr, nullptr, nullptr);
    }
    sqlite3_finalize(stmt);
    return true;
}

bool cDBSampler::ProgDetail_Delete(unsigned int ProgId, bool DelFather) {
    wxString Sql_Del = wxString::Format("DELETE FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d", ProgId);
    if (ExecuteSQL(Sql_Del, true) == true) {
        if (DelFather) {
            Sql_Del = wxString::Format("DELETE FROM " PROGMASTER_TABLENAME " WHERE ProgId = %d", ProgId);
            return ExecuteSQL(Sql_Del, true);
        }
    }
    return false;
}

bool cDBSampler::ProgDetail_Delete2(unsigned int ProgId, unsigned int DetailId) {
    wxString Sql_Del = wxString::Format("DELETE FROM " PROGDETAIL_TABLENAME " WHERE MasterId = %d AND DetailProg = %d", ProgId, DetailId);
    return ExecuteSQL(Sql_Del, true);
}

bool cDBSampler::ProgDetail_Select(int64_t masterId, int64_t detailProg, sCommand& item) {
    if (!m_db) return false;

    wxString strParNames;   // Generate Par? list dynamically
    for (byte i = 0; i < NUMOFPARAMS; i++) strParNames += wxString::Format(", Par%d", i);

    // Builds the SQL select query based on the primary key
    wxString sql = wxString::Format(
        "SELECT SubSys, Cmd, Pattern %s, DetailProg FROM %s WHERE MasterId = ? AND DetailProg >= ?;",
        strParNames, PROGDETAIL_TABLENAME
    );

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        ErrorShow(sqlite3_errmsg(m_db));
        return false;
    }

    // Binding of search parameters (WHERE)
    sqlite3_bind_int64(stmt, 1, masterId);
    sqlite3_bind_int64(stmt, 2, detailProg);

    bool recordFound = false;
    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        recordFound = true;

        item.m_MasterId     = masterId;

        // Standard data extraction
        item.m_SubSystem    = (eSubSysAcro)sqlite3_column_int(stmt, 0);
        item.m_Cmd          = sqlite3_column_int(stmt, 1);
        item.m_DetailProg   = sqlite3_column_int(stmt, 13);

        // Gestione del pattern di testo
        const char* patternText = (const char*)sqlite3_column_text(stmt, 2);
        if (patternText) {
            item.SetPattern(patternText);
        }

        // Estrazione dinamica dell'array dei parametri (a partire dalla colonna index 3)
        for (int i = 0; i < NUMOFPARAMS; i++) {
            item.m_Par[i] = sqlite3_column_int64(stmt, 3 + i);
        }
    } else if (rc == SQLITE_DONE) {
        recordFound = false; // Record non trovato, gestibile senza mostrare un errore di sistema
    } else {
        ErrorShow(sqlite3_errmsg(m_db)); // Errore effettivo del database durante l'esecuzione
    }

    sqlite3_finalize(stmt);
    return recordFound;
}
