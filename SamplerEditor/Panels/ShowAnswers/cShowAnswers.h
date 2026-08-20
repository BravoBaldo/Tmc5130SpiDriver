#pragma once
#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <wx/grid.h>
#include "stdwx.h"

#include "cpanPower.h"

enum eGrids {
    eGrid_Motors,
    eGrid_FSA,

    eGrid_TOP
};

class cAnswersShow : public wxAuiNotebook {
    wxGrid* m_Grids[eGrid_TOP];
    cpanPower* m_PanMotorPowers;
#if !defined(USE_MAIN_LOG)
    wxTextCtrl* m_txt_Log = nullptr;
#endif
    void Grid_AllReadOnly(wxGrid* Grid);
    void Grid_AutoSizeAll(wxGrid* Grid, bool setAsMin = true);

    void Log_Generic_InitEnd(wxGrid* Grid);

    void Log_Stepper_Init   (void);
    void Log_FSA_Init       (void);

    eCmdAnswer ShowAnswer_A(void* Answer, size_t AnswerLen);
public:
    cAnswersShow(wxWindow* parent);
    void Log_Stepper_Fill   (const TmcAnswer& Answer);
    void Log_FSA            (const FsaSingleAnswer& Answer) { m_PanMotorPowers->Fill(Answer); }
    void Log_FSA            (const TmcAnswer& Answer)       { m_PanMotorPowers->Fill(Answer); };

    bool SetAnswer(void* Answer, size_t AnswerLen);
    void AddSamplePages();
    int GetMotorSelected(void){return m_Grids[eGrid_Motors]->GetGridCursorRow(); }
};
