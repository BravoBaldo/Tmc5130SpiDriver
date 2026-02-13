#include "stdwx.h"
#include "SamplerCmdPanel.h"
#include "SamplerCmd.h"	//Commands_Fill

enum {
	ID_cho_Cmd = wxID_HIGHEST,
};

BEGIN_EVENT_TABLE(Sampler_StepParamsCtrl, wxPanel)
EVT_CHOICE(-1, OnChoice)
END_EVENT_TABLE()

void Sampler_StepParamsCtrl::OnChoice(wxCommandEvent& Evt) {
	int				Id = Evt.GetId();
}
Sampler_StepParamsCtrl::Sampler_StepParamsCtrl(	wxWindow* parent,
												wxWindowID		winid,
												const wxPoint& pos,
												const wxSize& size,
												long			style,
												const wxString& name
											) : wxPanel(parent, winid, pos, size, style, name)
{
	m_cho_SamplerCommand = new wxChoice(this, ID_cho_Cmd, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
	Commands_Fill(m_cho_SamplerCommand);
	m_cho_SamplerCommand->SetToolTip(_("PIC Commands XX"));
	m_cho_SamplerCommand->SetSelection(0);

	m_cho_Motor = new wxChoice(this, -1/*ID_cho_Cmd*/, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
	m_cho_Motor->SetToolTip(_("Motor List"));
	m_cho_Motor->Append("Motor X", (void*)0);
	m_cho_Motor->Append("Motor Y", (void*)1);
	m_cho_Motor->Append("Motor Z", (void*)2);
	m_cho_Motor->Append("Motor Probe", (void*)3);

	SIZER_STATDEBUG(sizMaster, "Main", wxHORIZONTAL);
		sizMaster->Add(m_cho_SamplerCommand, 1, wxALL | wxGROW, 5);
		sizMaster->Add(m_cho_Motor, 1, wxALL | wxGROW, 5);

	SetSizer(sizMaster); // use the sizer for layout
	sizMaster->Fit(this); // fit the dialog to the contents
	sizMaster->SetSizeHints(this); // set hints to honor min size
}
