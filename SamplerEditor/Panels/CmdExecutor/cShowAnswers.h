#pragma once
#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <wx/grid.h>
#include "../SysSampler.h"


enum eGrids {
    eGrid_Motors,

    eGrid_TOP
};

enum eSteppers {
    eStep_UpDwn,
    eStep_LR,
    eStep_Syringe,  
    eStep_Deposit,
    eStep_Needle,
    eStep_Spare,

    eStep_TOP
};

class cAnswersShow : public wxAuiNotebook {
    wxGrid* m_Grids[eGrid_TOP];
    void Grid_AllReadOnly(wxGrid* Grid);
    void Grid_AutoSizeAll(wxGrid* Grid, bool setAsMin = true);
    void Log_Generic_InitEnd(wxGrid* Grid);

    void Log_Stepper_Init(wxGrid* Grid, const wxFont& HeadFont);
    void Log_Stepper_Fill(wxGrid* Grid, void* Answer, size_t AnswerLen= sizeof(StepperAnswer));

	eCmdAnswer ShowAnswer_A(void* Answer, size_t AnswerLen);
public:
    cAnswersShow(wxWindow* parent);

	bool SetAnswer(void* Answer, size_t AnswerLen);
    void AddSamplePages();
};
