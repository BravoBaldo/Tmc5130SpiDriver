#pragma once
#include "wx/wx.h"
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>

class CoordDBctrl : public wxControl {
public:

    CoordDBctrl(wxWindow* parent, wxWindowID id = wxID_ANY
        , long style = wxSP_ARROW_KEYS, int min = 0, int max = 100, int initial = 0) : wxControl(parent, id) {

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
        SetLayout();

        wxCommandEvent Evt; OnChange(Evt);
        //ShowExtraControls(false);
        Layout();
        GetParent()->Layout();

    };
    int     GetValue()                  { return m_spinPos->GetValue(); }
    void    SetRange(int min, int max)  { m_spinPos->SetRange(min, max); }
    void    SetValue(int val)           { m_spinPos->SetValue(val); wxCommandEvent Evt; OnChange(Evt); }
    int     GetMin() const              { return m_spinPos->GetMin(); }
    int     GetMax() const              { return m_spinPos->GetMax(); }

    wxString        m_Prefix = "NAZMotPos";
    unsigned int    m_Theshold = 50000;
private:
    wxSpinCtrl*     m_spinPos;
    wxStaticText*   m_lblTrueVal;   //--------------------------
    wxSpinCtrl*     m_spinDBVal;

    wxStaticText*   m_lblDBName;    //"DB Name:"
    wxStaticText*   m_txtDBName;    //NAZMotPosXXX

    wxStaticText*   m_lblDescr;     //Description
    wxTextCtrl*     m_txtDescr;

    wxButton*       m_btnSave;

    wxSizer* m_mainSizer;

    void ShowExtraControls(bool show) {
        //show = true;
//#define USE_SHOW
#if defined(USE_SHOW)
        m_lblDescr  ->Show(show);
        m_lblTrueVal->Show(show);
        m_lblDBName ->Show(show);
        m_txtDBName->Show(show);
        m_spinDBVal ->Show(show);
        m_txtDescr  ->Show(show);
        m_btnSave   ->Show(show);
#else
        m_lblDescr  ->Enable(show);
        m_lblTrueVal->Enable(show);
        m_lblDBName ->Enable(show);
        m_txtDBName->Enable(show);
        m_spinDBVal ->Enable(show);
        m_txtDescr  ->Enable(show);
        m_btnSave   ->Enable(show);
#endif
        m_mainSizer->Layout();
        Layout(); PostSizeEventToParent();
    }

    void OnChange(wxCommandEvent& /*event*/);
    void OnUpdate(wxCommandEvent& /*event*/);

    void SetLayout(void) {
        m_mainSizer = new wxBoxSizer(wxVERTICAL);

        wxGridBagSizer* gbSizer1 = new wxGridBagSizer(2, 2);
        gbSizer1->SetFlexibleDirection(wxBOTH);
        gbSizer1->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
        gbSizer1->Add(m_spinPos, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL, 0);

        gbSizer1->Add(m_lblTrueVal, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_spinDBVal, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 0);

        gbSizer1->Add(m_lblDBName, wxGBPosition(1, 0), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_txtDBName, wxGBPosition(1, 1), wxGBSpan(1, 2), wxEXPAND, 0);
        gbSizer1->Add(m_lblDescr, wxGBPosition(2, 0), wxGBSpan(1, 1), wxALL, 0);
        gbSizer1->Add(m_txtDescr, wxGBPosition(2, 1), wxGBSpan(1, 2), wxEXPAND, 0);
        gbSizer1->Add(m_btnSave, wxGBPosition(3, 2), wxGBSpan(1, 1), wxALIGN_RIGHT, 0);

        gbSizer1->AddGrowableCol(2);


        m_mainSizer->Add(gbSizer1, 1, wxEXPAND, 5);
        SetSizer(m_mainSizer);
        Layout(); PostSizeEventToParent();
    }

    void SetLayout2(void) {
        m_mainSizer = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer* m_condSizerF = new wxFlexGridSizer(0, 2, 0, 0);
        m_condSizerF->AddGrowableCol(1);
        m_condSizerF->SetFlexibleDirection(wxBOTH);
        m_condSizerF->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);
#define OrgSolXX
#if defined(OrgSolXX)
        m_condSizerF->Add(m_spinPos,    1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(0, 0, 1, wxEXPAND, 5);
        m_condSizerF->Add(m_lblDBName,  0, wxALIGN_RIGHT | wxALL, 0);
        m_condSizerF->Add(m_spinDBVal,  1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(m_lblDescr,   0, wxALIGN_RIGHT | wxALL, 5);
        m_condSizerF->Add(m_txtDescr,   1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(0,0,          1, wxEXPAND, 5);
        m_condSizerF->Add(m_btnSave,    1, wxALIGN_RIGHT | wxALL, 5);
#else
        m_condSizerF->Add(m_spinPos, 1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(m_lblDBName, 0, wxALIGN_LEFT | wxALL, 0);
        m_condSizerF->Add(m_spinDBVal, 1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(m_txtDescr, 1, wxALL | wxEXPAND, 0);
//        m_condSizerF->Add(0, 0, 1, wxEXPAND, 5);
        m_condSizerF->Add(m_lblDescr, 0, wxALIGN_RIGHT | wxALL, 5);
        m_condSizerF->Add(m_btnSave, 1, wxALIGN_RIGHT | wxALL, 5);
#endif
        m_mainSizer->Add(m_condSizerF, 1, wxEXPAND, 5);
        SetSizer(m_mainSizer);

        wxWindow* w = this;
        while ((w = w->GetParent())!=nullptr){
            w->Layout();
        }
    }
};

