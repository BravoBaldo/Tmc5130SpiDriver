#pragma once
#include "wx/wx.h"
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>

//#define COORDDB_IN_TEST

class CoordDBctrl : public wxControl {
public:
    CoordDBctrl(wxWindow* parent, wxWindowID id = wxID_ANY
        , long style = wxSP_ARROW_KEYS, int min = 0, int max = 100, int initial = 0)
        : wxControl(parent, id), m_shownPrevious(false) {

        m_spinPos = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style, min, max, initial);
            m_spinPos->Bind(wxEVT_SPINCTRL, &CoordDBctrl::OnChange, this);
            m_spinPos->Bind(wxEVT_TEXT, &CoordDBctrl::OnChange, this); // Gestisce anche l'input manuale da tastiera

        m_lblTrueVal = new wxStaticText(this, wxID_ANY, _("True Val -->"), wxDefaultPosition, wxDefaultSize, 0);
        m_lblTrueVal->Wrap(-1);

        m_lblDBName = new wxStaticText(this, wxID_ANY, _("DB Name:"), wxDefaultPosition, wxDefaultSize, 0);
        m_lblDBName->Wrap(-1);

        m_txtDBName = new wxStaticText  (this, wxID_ANY, "NAZMotPosXXX"); m_txtDBName->Wrap(10);
        m_spinDBVal = new wxSpinCtrl    (this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style, min, max, initial);

        m_lblDescr  = new wxStaticText(this, wxID_ANY, "Description:"); m_lblDescr->Wrap(-1);
        m_txtDescr  = new wxTextCtrl    (this, wxID_ANY, "Insert a text...", wxDefaultPosition, wxSize(-1, 50), wxALIGN_LEFT | wxTE_MULTILINE);

        m_btnSave   = new wxButton      (this, wxID_ANY, "Update DB");
            m_btnSave->Bind(wxEVT_BUTTON, &CoordDBctrl::OnUpdate, this);
        //--------------------------------------------------------------
        SetLayout();
        //-------------------------------------------------------
        m_shownPrevious = ChkThreshold();// ((unsigned int)m_spinPos->GetValue() > m_Theshold);
        UpdateInterface(true);
    };

    virtual ~CoordDBctrl() = default;

    inline bool ChkThreshold()  {return ((unsigned int)m_spinPos->GetValue() > m_Theshold); }

    int     GetValue()                  { return m_spinPos->GetValue(); }
    void    SetRange(int min, int max)  { m_spinPos->SetRange(min, max); }
    void    SetValue(int val)           { m_spinPos->SetValue(val);
                                          wxCommandEvent Evt; OnChange(Evt);
                                          UpdateInterface();
                                        }
    int     GetMin() const              { return m_spinPos->GetMin(); }
    int     GetMax() const              { return m_spinPos->GetMax(); }

    wxString        m_Prefix = "NAZMotPos";
    unsigned int    m_Theshold = 50000;
private:
    bool            m_shownPrevious = false;
    wxSpinCtrl*     m_spinPos;
    wxStaticText*   m_lblTrueVal;   //--------------------------
    wxSpinCtrl*     m_spinDBVal;

    wxStaticText*   m_lblDBName;    //"DB Name:"
    wxStaticText*   m_txtDBName;    //NAZMotPosXXX

    wxStaticText*   m_lblDescr;     //Description
    wxTextCtrl*     m_txtDescr;

    wxButton*       m_btnSave;

    void ScaleResize(wxSizer* sz, bool TopToo = false){
        if (sz) {
            sz->Layout();
            wxSize minSize = sz->GetMinSize();    // Ricalcola la dimensione minima necessaria per contenere SOLO gli elementi visibili
            SetMinSize(minSize);
            SetSize(minSize);
        }
        wxWindow* win = GetParent();
        while (win) {
            // Se incontriamo un pannello intermedio (come CmdParLabel2), aggiorna il suo sizer
            wxSizer* szl = win->GetSizer();
            if (szl) {
                szl->Layout();
                // Aggiorna la dimensione minima del pannello basandoti sul suo sizer interno
                win->SetMinSize(szl->GetMinSize());
            }

            // Se siamo arrivati al Frame principale (Top Level Window)
            if (win->IsTopLevel()) {
                if (TopToo) {   //Top Level Window too
                    wxFrame* frame = wxDynamicCast(win, wxFrame);
                    if (frame) {
                        wxSizer* szf = frame->GetSizer();
                        if (szf) szf->Layout();
                        frame->Fit(); // Costringe il Frame ad allargarsi o stringersi fisicamente sulla scrivania!
                    }
                }
                break; // Usciamo dal ciclo una volta aggiornato il Frame
            }
            win = win->GetParent();
        }

    }

    void UpdateInterface(bool forceSizer = false, bool TopToo = false) {
        bool show = ChkThreshold();// ((unsigned int)m_spinPos->GetValue() > m_Theshold);
        m_spinDBVal->Show(show);
        m_txtDescr->Show(show);
        m_btnSave->Show(show);

        m_lblDescr->Show(show);
        m_lblTrueVal->Show(show);
        m_lblDBName->Show(show);
        m_txtDBName->Show(show);

        wxSizer* sz = GetSizer();
        if (show != m_shownPrevious || forceSizer) {
            m_shownPrevious = show;
            ScaleResize(sz, TopToo);
        } else {
            if (sz) sz->Layout();
            Refresh();
        }
    }





#if defined(COORDDB_IN_TEST)
    void OnChange(wxCommandEvent& Evt) {
        UpdateInterface();
        Evt.Skip();
    }

    void OnUpdate(wxCommandEvent& /*event*/) {}
#else
    void OnChange(wxCommandEvent& Evt);
    void OnUpdate(wxCommandEvent& Evt);
#endif



    void SetLayout3(void) {
        wxBoxSizer* m_mainSizer = new wxBoxSizer(wxVERTICAL);
        m_mainSizer->Add(m_spinPos,     0, wxALL, 2);
        m_mainSizer->Add(m_spinDBVal,   0, wxALL, 2);
        m_mainSizer->Add(m_txtDescr,    0, wxALL, 2);
        m_mainSizer->Add(m_btnSave,     0, wxALL, 2);

        m_mainSizer->Add(m_lblDescr,    0, wxALL, 2);
        m_mainSizer->Add(m_lblTrueVal,  0, wxALL, 2);
        m_mainSizer->Add(m_lblDBName,   0, wxALL, 2);
        m_mainSizer->Add(m_txtDBName,   0, wxALL, 2);

        SetSizerAndFit(m_mainSizer);    //SetSizer(m_mainSizer);
    }

    void SetLayout(void) {
        wxBoxSizer* m_mainSizer = new wxBoxSizer(wxVERTICAL);

        wxGridBagSizer* gbSizer1 = new wxGridBagSizer(2, 2);
        gbSizer1->SetFlexibleDirection(wxBOTH);
        gbSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

        gbSizer1->Add(m_spinPos,    wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_lblTrueVal, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_spinDBVal,  wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_lblDBName,  wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_txtDBName,  wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND, 0);
        gbSizer1->Add(m_lblDescr,   wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_txtDescr,   wxGBPosition(2, 1), wxGBSpan(1, 2), wxEXPAND, 0);
        gbSizer1->Add(m_btnSave,    wxGBPosition(3, 2), wxGBSpan(1, 1), wxALIGN_RIGHT, 0);

//        gbSizer1->AddGrowableCol(2);

        m_mainSizer->Add(gbSizer1, 0, wxEXPAND, 5);
        SetSizerAndFit(m_mainSizer);
        UpdateInterface();
    }
};

