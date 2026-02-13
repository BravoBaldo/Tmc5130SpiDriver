#pragma once
#include "wx/wx.h"

class DBMasterCtrl : public wxPanel {
	wxChoice* m_cho_SamplerCommand	= nullptr;	
	wxChoice* m_cho_Motor			= nullptr;
	DECLARE_EVENT_TABLE()
protected:
	//virtual void OnChoice(wxCommandEvent& Evt) = 0;
public:
	~DBMasterCtrl() { };
	DBMasterCtrl(	wxWindow*		parent,
					wxWindowID		winid = wxID_ANY,
					const wxPoint&	pos = wxDefaultPosition,
					const wxSize&	size = wxDefaultSize,
					long			style = wxTAB_TRAVERSAL | wxNO_BORDER,
					const wxString& name = wxPanelNameStr
				);

	void OnChoice(wxCommandEvent& Evt);
};
