#pragma once
#include <wx/wx.h>
#include <wx/grid.h>
#include "stdwx.h"

#include "cpanPower.h"

class cpanSteppers : public wxPanel {
    wxGrid*     m_grid_Motors   = nullptr;
    void		InitGrid();
    void		OnGridMouseMotion(wxMouseEvent& event);
    void    Fill(const FsaSingleAnswer& Answer);
    void    Fill(const TmcAnswer& Answer);
public:
    cpanSteppers    (wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString);
    ~cpanSteppers   () {};
    void Fill       (const AnswerHeader* ptrHeader);
    int  GetMotorSelected() { return m_grid_Motors ? m_grid_Motors->GetGridCursorRow() : 0; }
};
