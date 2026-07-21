#pragma once
#include "wx/wx.h"
#include <wx/spinctrl.h>
#include "cDBSampler.h"

class DBRoutineCtrl : public wxControl {
    wxSizer*        m_mainSizer;
    wxChoice*       m_RoutineList;
    void            FillRoutines(void);

    void gGetIndexFromClientData(wxChoice* Cho, int DataInt) {
        if (Cho) {
            size_t j = Cho->GetCount();
            for (size_t i = 0; i < j; i++) {
                int x = static_cast<int>(reinterpret_cast<intptr_t>(Cho->GetClientData(i)));
                if (x == DataInt) {
                    Cho->SetSelection(i);
                    return;
                }
            }
        }
    }

    void* gGetSelectedClientData(wxChoice* Cho) {
        if (Cho) {
            int Sel = Cho->GetSelection();
            if (Sel >= 0)
                return Cho->GetClientData(Sel);
        }
        return NULL;
    }
public:
    DBRoutineCtrl(wxWindow* parent, wxWindowID id = wxID_ANY) : wxControl(parent, id) {
        m_RoutineList = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
            FillRoutines();
        SetLayout();
        Layout();
        GetParent()->Layout();
    }

    void SetLayout(void) {
        m_mainSizer = new wxBoxSizer(wxHORIZONTAL);// wxVERTICAL);
        m_mainSizer->Add(m_RoutineList, 1, wxEXPAND | wxALL, 0);
        SetSizer(m_mainSizer);
    }
    int GetValue()                  { return static_cast<int>(reinterpret_cast<intptr_t>(gGetSelectedClientData(m_RoutineList))); }
    void SetValue(int val)          { gGetIndexFromClientData(m_RoutineList, val); }
};
