#pragma once
#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <wx/grid.h>
#include "stdwx.h"

#include "cpanPower.h"
#include "cEnergy.h"
#include "cpanSteppers.h"

class cAnswersShow : public wxPanel {
	cEnergy*	    m_Energy            = nullptr;
    wxAuiNotebook*  m_NoteBook          = nullptr;

    cpanPower*      m_PanMotorPowers    = nullptr;  //"FSA2"
    cpanSteppers*   m_panSteppers       = nullptr;  //"Steppers"
#if !defined(USE_MAIN_LOG)
    wxTextCtrl*     m_txt_Log           = nullptr;  //Last Status
#endif
    void AddSamplePages     (wxAuiNotebook* NB);
public:
    cAnswersShow            (wxWindow* parent);

    bool SetAnswer          (const AnswerHeader* Answer, size_t AnswerLen);
    int  GetMotorSelected() { return m_panSteppers ? m_panSteppers->GetMotorSelected() : 0; }
};
