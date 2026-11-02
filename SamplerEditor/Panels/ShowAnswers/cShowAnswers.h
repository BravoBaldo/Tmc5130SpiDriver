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

    //ToDo: AAA a seconda del pannello visualizzato...
    int  GetMotorSelected() { 
        wxWindow* paginaGenerica = m_NoteBook->GetCurrentPage();
        if(paginaGenerica == m_panSteppers)
            return m_panSteppers ? m_panSteppers->GetMotorSelected() : 0;
        else if (paginaGenerica == m_PanMotorPowers)
            return m_PanMotorPowers ? m_PanMotorPowers->GetMotorSelected() : 0;
        //m_panSteppers = new cpanSteppers(m_NoteBook);	m_NoteBook->AddPage(m_panSteppers, _("Steppers"));
        //m_PanMotorPowers = new cpanPower(m_NoteBook);	m_NoteBook->AddPage(m_PanMotorPowers, _("FSA"));
        return -1;
    }
};
