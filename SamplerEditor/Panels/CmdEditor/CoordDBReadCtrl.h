#pragma once
#include "wx/wx.h"
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <vector>
#include <algorithm>

/*
ToDo: To refine:
    If value <=50000, use wxSpinCtrl
    else    wxComboBox

*/



//#define DBROUTINEREAD_IN_TEST

class CoordDBReadctrl : public wxControl {
    wxSizer* m_mainSizer;
    wxComboBox* m_RoutineList; // Sostituito wxChoice con wxComboBox

#if defined(DBROUTINEREAD_IN_TEST)
    // Aggiunto il parametro booleano per gestire l'ordinamento richiesto
    void FillRoutines(bool SortByName = false) {
        m_RoutineList->Clear();

        if (SortByName) {
            // Struttura temporanea per ordinare alfabeticamente mantenendo i dati associati
            struct RoutineData {
                wxString stringa;
                int id;
            };
            std::vector<RoutineData> lista;
            for (int i = 2001; i < 2050; i++) {
                lista.push_back({ wxString::Format("Test Routine %d", i), i });
            }
            // Ordinamento crescente in base al testo della stringa
            std::sort(lista.begin(), lista.end(), [](const RoutineData& a, const RoutineData& b) {
                return a.stringa < b.stringa;
                });

            for (const auto& item : lista) {
                m_RoutineList->Append(item.stringa, reinterpret_cast<void*>(static_cast<intptr_t>(item.id)));
            }
        } else {
            // Ordine nativo/originale numerico
            for (int i = 2001; i < 2050; i++) {
                m_RoutineList->Append(
                    wxString::Format("Test Routine %d", i),
                    reinterpret_cast<void*>(static_cast<intptr_t>(i)));
            }
        }
    }
#else
    void            FillRoutines(bool SortByNum = true);
#endif

    void gGetIndexFromClientData(wxComboBox* Cho, int DataInt) {
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

    void* gGetSelectedClientData(wxComboBox* Cho) {
        if (Cho) {
            int Sel = Cho->GetSelection();
            if (Sel >= 0)
                return Cho->GetClientData(Sel);
        }
        return NULL;
    }

    // Gestore dell'evento dropdown: riordina al volo all'apertura del menu a tendina
    void OnDropdownAperto(wxCommandEvent& event) {
        wxMouseState mouseState = wxGetMouseState();

        // Salviamo i dati correnti per riapplicarli dopo il Clear
        void* dati_selezionati = gGetSelectedClientData(m_RoutineList);

        if (mouseState.ShiftDown()|| mouseState.ControlDown()) {
            FillRoutines(true); // Shift-Click: Ordine crescente (alfabetico)
        }
        else {
            // Click normale: Ordine nativo
            FillRoutines(false);
        }

        // Ripristiniamo la selezione corretta cercando il ClientData salvato
        if (dati_sevisivi(dati_selezionati)) {
            int targetVal = static_cast<int>(reinterpret_cast<intptr_t>(dati_selezionati));
            gGetIndexFromClientData(m_RoutineList, targetVal);
        }

        event.Skip(); // Permette a wxWidgets di mostrare fisicamente la tendina a schermo
    }

    bool dati_sevisivi(void* ptr) { return ptr != NULL; }

public:
    CoordDBReadctrl(wxWindow* parent, wxWindowID id = wxID_ANY) : wxControl(parent, id) {
        // Creato wxComboBox con lo stile wxCB_READONLY per emulare perfettamente il comportamento grafico del wxChoice
        m_RoutineList = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

        FillRoutines();
        SetLayout();
        Layout();
        GetParent()->Layout();

        // Bind dell'evento macro per intercettare l'apertura del dropdown
        m_RoutineList->Bind(wxEVT_COMBOBOX_DROPDOWN, &CoordDBReadctrl::OnDropdownAperto, this);
    }

    void SetLayout(void) {
        m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
        m_mainSizer->Add(m_RoutineList, 1, wxEXPAND | wxALL, 0);
        SetSizer(m_mainSizer);
    }
    int GetValue() { return static_cast<int>(reinterpret_cast<intptr_t>(gGetSelectedClientData(m_RoutineList))); }
    void SetValue(int val) { gGetIndexFromClientData(m_RoutineList, val); }
};
