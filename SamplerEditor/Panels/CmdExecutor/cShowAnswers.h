#pragma once
#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <wx/grid.h>
#include "stdwx.h"

enum eGrids {
    eGrid_Motors,

    eGrid_TOP
};
/*
enum eSteppers {    //
    eStep_UpDwn,
    eStep_LR,
    eStep_Syringe,  
    eStep_Deposit,
    eStep_Needle,
    eStep_Spare,

    eStep_TOP
};
*/

class cAnswersShow : public wxAuiNotebook {
    wxGrid* m_Grids[eGrid_TOP];
#if !defined(USE_MAIN_LOG)
    wxTextCtrl* m_txt_Log = nullptr;
#endif
    void Grid_AllReadOnly(wxGrid* Grid);
    void Grid_AutoSizeAll(wxGrid* Grid, bool setAsMin = true);
    void Log_Generic_InitEnd(wxGrid* Grid);

    void Log_Stepper_Init(const wxFont& HeadFont);
    

	eCmdAnswer ShowAnswer_A(void* Answer, size_t AnswerLen);
public:
    cAnswersShow(wxWindow* parent);
    void Log_Stepper_Fill(const StepperAnswer& Answer);
    void Log_Stepper_Fill(const TmcAnswer& Answer);

	bool SetAnswer(void* Answer, size_t AnswerLen);
    void AddSamplePages();
};
