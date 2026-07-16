#pragma once
#include "wx/wx.h"
#include <wx/spinctrl.h>


class CoordDBctrl : public wxControl {
public:

    CoordDBctrl(wxWindow* parent, wxWindowID id = wxID_ANY
        , long style = wxSP_ARROW_KEYS, int min = 0, int max = 100, int initial = 0) : wxControl(parent, id) {
        m_lblPos = new wxStaticText(this, wxID_ANY, "Position:");   m_lblPos->Wrap(-1);

        m_spinPos = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, style, min, max, initial);
            m_spinPos->Bind(wxEVT_SPINCTRL, &CoordDBctrl::OnPositionChange, this);
            m_spinPos->Bind(wxEVT_TEXT, &CoordDBctrl::OnPositionChange, this); // Gestisce anche l'input manuale da tastiera

        m_lblDBName = new wxStaticText  (this, wxID_ANY, "NAZMotPosXXXX:"); m_lblDBName->Wrap(-1);
        m_spinDBVal = new wxSpinCtrl    (this, wxID_ANY, "0");

        m_lblDescr  = new wxStaticText(this, wxID_ANY, "Descrizione:"); m_lblDescr->Wrap(-1);
        m_txtDescr  = new wxTextCtrl    (this, wxID_ANY, "Inserisci testo...");

        m_btnSave   = new wxButton      (this, wxID_ANY, "Aggiorna DB");
        SetLayout();
        ShowExtraControls(false); 
        Layout();

    };
    int GetValue()                  { return m_spinPos->GetValue(); }
    void SetRange(int min, int max) { m_spinPos->SetRange(min, max); }
    void SetValue(int val)          { m_spinPos->SetValue(val);}
    int GetMin() const              { return m_spinPos->GetMin(); }
    int GetMax() const              { return m_spinPos->GetMax(); }

    wxString        m_Prefix = "NAZMotPos";
    unsigned int    m_Theshold = 50000;
private:
    wxStaticText*   m_lblPos;
    wxSpinCtrl*     m_spinPos;

    wxStaticText*   m_lblDBName;
    wxSpinCtrl*     m_spinDBVal;

    wxStaticText*   m_lblDescr;
    wxTextCtrl*     m_txtDescr;

    wxButton*       m_btnSave;

    wxBoxSizer* m_mainSizer;

    void ShowExtraControls(bool show) {
        show = true;
        m_lblDescr  ->Show(show);
        m_lblDBName ->Show(show);
        m_spinDBVal ->Show(show);
        m_txtDescr  ->Show(show);
        m_btnSave   ->Show(show);

        m_mainSizer->Layout();    // Forza il sizer a ricalcolare le dimensioni e riposizionare i controlli visibili
        GetParent()->Layout();
    }

    void OnPositionChange(wxCommandEvent& /*event*/) {
        bool up = m_spinPos->GetValue() > (int)m_Theshold;
        if (up) m_lblDBName->SetLabel(wxString::Format("%s%03d:", m_Prefix, m_spinPos->GetValue()-m_Theshold));
        ShowExtraControls(up);
    }

    void SetLayout(void) {
        m_mainSizer = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer* m_condSizerF = new wxFlexGridSizer(0, 2, 0, 0);
        m_condSizerF->AddGrowableCol(1);
        m_condSizerF->SetFlexibleDirection(wxBOTH);
        m_condSizerF->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

        m_condSizerF->Add(m_lblPos, 0, wxALIGN_RIGHT | wxALL, 0);
        m_condSizerF->Add(m_spinPos, 1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(m_lblDBName, 0, wxALIGN_RIGHT | wxALL, 0);
        m_condSizerF->Add(m_spinDBVal, 1, wxALL | wxEXPAND, 0);
        m_condSizerF->Add(m_lblDescr, 0, wxALIGN_RIGHT | wxALL, 5);
            m_condSizerF->Add(m_txtDescr, 1, wxALL | wxEXPAND, 0);
            m_condSizerF->Add(0, 0, 1, wxEXPAND, 5);
            m_condSizerF->Add(m_btnSave, 1, wxALIGN_RIGHT | wxALL, 5);

        m_mainSizer->Add(m_condSizerF, 1, wxEXPAND, 5);

        SetSizer(m_mainSizer);
    }
};

