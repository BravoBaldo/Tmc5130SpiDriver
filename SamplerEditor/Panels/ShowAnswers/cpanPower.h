#pragma once
#include <wx/wx.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>


class cpanPower : public wxPanel {
	private:
		wxGrid*		m_grid_Motors = nullptr;
		void		InitGrid			();
		void		OnGridMouseMotion	(wxMouseEvent& event);
		void Fill(const FsaSingleAnswer& Answer);
		void Fill(const TmcAnswer& Answer);
public:
		cpanPower( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~cpanPower()	{};
		void Fill(const AnswerHeader* ptrHeader);

};
