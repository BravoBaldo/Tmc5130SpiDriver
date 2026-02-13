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
    int rc = sqlite3_open(filename, &m_db);
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
    wxString	Translation;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        long itemIndex = listCtrl->InsertItem(rowIndex, "");
        for (int col = 0; col < colCount; col++) {
            const char* val = (const char*)sqlite3_column_text(stmt, col);
           // Translation = ((col == Fld2Translate) ? _(val) : val);
            wxString cellValue = (val) ? wxString::FromUTF8(val) : "";
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

void cDBSampler::ProgDetail_Fill(unsigned int ProgId, wxListCtrl* ListCtrl, bool DoResize, int Fld2Translate) {
    ListCtrl_FillFromSql(ListCtrl, wxString::Format(
                                    // Same order of cDetailListCtrl::PrgDetail_FillListItem
                                    //		StepId                      	  CmdTyp  SubCmd  [0]   [1]   [2]   [3]   ProgramId I2CAddr
                         //SELECT MasterId, DetailProg,        Motor, Cmd, Pattern, Cnt, Par1, Par2, Par3, Par4, Par5, Par6, Par7
                                    "SELECT DetailProg AS [%s], Motor, Cmd, Pattern AS [%s], Cnt, Par1, Par2, Par3, Par4, Par5, Par6, Par7, MasterId "
                                    " FROM " PROGDETAIL_TABLENAME 
                                    " WHERE MasterId = %d ORDER BY DetailProg"
                                    , _("N")
                                    , _("Command")
                                    , ProgId
                                )
                        , DoResize, Fld2Translate);
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

        wxString SqlCmd = wxString::Format(
            "INSERT INTO %s (MasterId, DetailProg, Motor, Cmd, Pattern, Cnt, Par1, Par2, Par3, Par4, Par5, Par6, Par7) "
                            "SELECT %d, DetailProg, Motor, Cmd, Pattern, Cnt, Par1, Par2, Par3, Par4, Par5, Par6, Par7 "
            "FROM %s WHERE MasterId = %d;",
            PROGDETAIL_TABLENAME, ProgIdNew,
            PROGDETAIL_TABLENAME, ProgIdOld
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
    const char* sql_create =    "CREATE TABLE IF NOT EXISTS " PROGDETAIL_TABLENAME " ("
                                    "MasterId      INTEGER NOT NULL,"
                                    "DetailProg    INTEGER NOT NULL,"
                                    "Motor         INTEGER NOT NULL,"
                                    "Cmd           INTEGER NOT NULL,"
                                    "Pattern       TEXT NOT NULL DEFAULT '',"
                                    "Cnt           INTEGER NOT NULL DEFAULT 0,"
                                    "Par1          INTEGER,"
                                    "Par2          INTEGER,"
                                    "Par3          INTEGER,"
                                    "Par4          INTEGER,"
                                    "Par5          INTEGER,"
                                    "Par6          INTEGER,"
                                    "Par7          INTEGER,"
                                    "PRIMARY KEY (MasterId, DetailProg),"   // Definizione Chiave Primaria Composta
                                    "FOREIGN KEY (MasterId) REFERENCES " PROGMASTER_TABLENAME " (ProgId) "  // Definizione Foreign Key con DELETE CASCADE
                                    "ON DELETE CASCADE ON UPDATE CASCADE"
                                ") WITHOUT ROWID;"; // Ottimizzazione per tabelle con PK composta
    return CreateTable(sql_create);
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
    if (!ProgMaster_Insert("Baldone")) return false;

    return true;
}

#if defined(USEXML)
bool cDBSampler::ProgMaster_Export2(unsigned int ProgId, const wxString& filePath) {
    if (!m_db) return false;

    tinyxml2::XMLDocument xml;
    // Dichiarazione XML standard
    XMLDeclaration* decl = xml.NewDeclaration();
    xml.InsertFirstChild(decl);

    XMLNode* root = xml.NewElement("ProgramExport");
    xml.InsertEndChild(root);

    sqlite3_stmt* stmtMaster, * stmtSlave;

    // 1. Recupero dati MASTER
    wxString sqlMaster = wxString::Format("SELECT * FROM %s WHERE ProgId = ?;", PROGMASTER_TABLENAME);
    if (sqlite3_prepare_v2(m_db, sqlMaster.utf8_str(), -1, &stmtMaster, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmtMaster, 1, ProgId);

    if (sqlite3_step(stmtMaster) == SQLITE_ROW) {
        XMLElement* masterElement = xml.NewElement("Master");
        root->InsertEndChild(masterElement);

        int cols = sqlite3_column_count(stmtMaster);
        for (int i = 0; i < cols; i++) {
            const char* colName = sqlite3_column_name(stmtMaster, i);
            const char* colVal = (const char*)sqlite3_column_text(stmtMaster, i);
            masterElement->SetAttribute(colName, colVal ? colVal : "");
        }

        // 2. Recupero dati SLAVE
        XMLElement* detailsElement = xml.NewElement("Details");
        masterElement->InsertEndChild(detailsElement);

        wxString sqlSlave = wxString::Format("SELECT * FROM %s WHERE MasterId = ? ORDER BY DetailProg;", PROGDETAIL_TABLENAME);
        if (sqlite3_prepare_v2(m_db, sqlSlave.utf8_str(), -1, &stmtSlave, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtSlave, 1, ProgId);

            while (sqlite3_step(stmtSlave) == SQLITE_ROW) {
                XMLElement* stepElement = xml.NewElement("Step");
                detailsElement->InsertEndChild(stepElement);

                int sCols = sqlite3_column_count(stmtSlave);
                for (int j = 0; j < sCols; j++) {
                    const char* sColName = sqlite3_column_name(stmtSlave, j);
                    const char* sColVal = (const char*)sqlite3_column_text(stmtSlave, j);
                    stepElement->SetAttribute(sColName, sColVal ? sColVal : "");
                }
            }
            sqlite3_finalize(stmtSlave);
        }
    } else {
        sqlite3_finalize(stmtMaster);
        return false;
    }

    sqlite3_finalize(stmtMaster);

    // 3. Salvataggio su file
    XMLError eResult = xml.SaveFile(filePath.utf8_str());
    return (eResult == XML_SUCCESS);
}

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

bool cDBSampler::ProgMaster_Print(int ProgId, const wxString& /*ProgName*/, const wxString& filePath) {
    if (!m_db) return false;

    wxFFile file(filePath, "w"); // Apre in modalita' scrittura
    if (!file.IsOpened()) return false;

    sqlite3_stmt* stmtMaster, * stmtSlave;

    // 1. Recupero e scrittura dati MASTER
    wxString sqlMaster = wxString::Format("SELECT * FROM %s WHERE ProgId = ?;", PROGMASTER_TABLENAME);
    if (sqlite3_prepare_v2(m_db, sqlMaster.utf8_str(), -1, &stmtMaster, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmtMaster, 1, ProgId);

    if (sqlite3_step(stmtMaster) == SQLITE_ROW) {
        file.Write(wxString::Format("=== PROGRAMMA MASTER (ID: %d) ===\n", ProgId));

        int cols = sqlite3_column_count(stmtMaster);
        for (int i = 0; i < cols; i++) {
            wxString colName = wxString::FromUTF8(sqlite3_column_name(stmtMaster, i));
            wxString colVal = wxString::FromUTF8((const char*)sqlite3_column_text(stmtMaster, i));
            file.Write(wxString::Format("%s: %s\n", colName, colVal));
        }
        file.Write("\n--- DETTAGLI SLAVE ---\n");

        // 2. Recupero e scrittura dati SLAVE
        wxString sqlSlave = wxString::Format("SELECT * FROM %s WHERE MasterId = ? ORDER BY DetailProg;", PROGDETAIL_TABLENAME);
        if (sqlite3_prepare_v2(m_db, sqlSlave.utf8_str(), -1, &stmtSlave, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmtSlave, 1, ProgId);

            while (sqlite3_step(stmtSlave) == SQLITE_ROW) {
                int sCols = sqlite3_column_count(stmtSlave);
                wxString line = "> ";
                for (int j = 0; j < sCols; j++) {
                    wxString sColVal = wxString::FromUTF8((const char*)sqlite3_column_text(stmtSlave, j));
                    //line += sColVal + (j < sCols - 1 ? " | " : "");
                  line += "\"" + sColVal + "\"" + (j < sCols - 1 ? ";" : ""); //Per uscita CSV
                }
                file.Write(line + "\n");
            }
            sqlite3_finalize(stmtSlave);
        }
    } else {
        sqlite3_finalize(stmtMaster);
        return false;
    }

    sqlite3_finalize(stmtMaster);
    file.Close();
    return true;
}




#define TENTA3
bool cDBSampler::ProgDetail_Renum(unsigned int ProgId) {
#ifdef TENTA3
    if (!m_db) return false;
    if (sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) return false;

    wxString sqlTemp = wxString::Format(
        "WITH NewSequence AS ("
        "  SELECT MasterId, DetailProg, "
        "  (ROW_NUMBER() OVER (PARTITION BY MasterId ORDER BY DetailProg) * 3) AS NewVal "
        "  FROM SAM_ProgDetail "
        "  WHERE MasterId = %u"
        ") "
        "UPDATE SAM_ProgDetail "
        "SET DetailProg = -(SELECT NewVal FROM NewSequence "
        "                  WHERE NewSequence.MasterId = SAM_ProgDetail.MasterId "
        "                  AND NewSequence.DetailProg = SAM_ProgDetail.DetailProg) "
        "WHERE MasterId = %u;", ProgId, ProgId
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

bool cDBSampler::ProgDetail_Insert(const cCmdStepper& item, bool AllowRenum ) {
    if (!m_db) return false;

    // Query SQL con clausola ON CONFLICT per gestire l'aggiornamento automatico
    wxString sql = wxString::Format(
        "INSERT INTO %s (MasterId, DetailProg, Motor, Cmd, Pattern, Cnt, Par1, Par2, Par3, Par4, Par5, Par6, Par7) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(MasterId, DetailProg) DO UPDATE SET "
        "Motor=excluded.Motor, Cmd=excluded.Cmd, Pattern=excluded.Pattern, "
        "Cnt=excluded.Cnt, Par1=excluded.Par1, Par2=excluded.Par2, Par3=excluded.Par3, "
        "Par4=excluded.Par4, Par5=excluded.Par5, Par6=excluded.Par6, Par7=excluded.Par7;",
        PROGDETAIL_TABLENAME
    );

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(m_db, sql.utf8_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // Binding dei parametri dall'oggetto cCmdStepper
    sqlite3_bind_int64(stmt, 1, item.m_MasterId);
    sqlite3_bind_int64(stmt, 2, item.m_DetailProg);
    sqlite3_bind_int(stmt, 3, item.m_Motor);
    sqlite3_bind_int(stmt, 4, item.m_Cmd);
    sqlite3_bind_text(stmt, 5, item.m_Pattern.utf8_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, item.m_Cnt);

    for (int i = 0; i < 7; i++) {
        sqlite3_bind_int64(stmt, 7 + i, item.m_Par[i]);
    }

    // Esecuzione
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (AllowRenum)
        ProgDetail_Renum(item.m_MasterId);    //Update don't require renumbering

    return (rc == SQLITE_DONE);
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
