#include "stdwx.h"
#include "wx/artprov.h"
#include "CmdExecutorCtrl.h"

enum {
	ID_Btn_ExecAll = wxID_HIGHEST,
	ID_Btn_ExecStep,
	ID_Btn_Panic,
};

BEGIN_EVENT_TABLE(CmdExecutorCtrl, wxPanel)
	EVT_BUTTON(-1, CmdExecutorCtrl::OnBtnCommands)
END_EVENT_TABLE()

void CmdExecutorCtrl::SetEditorAndDB(CmdEditorCtrl* ptrEditor, cDetailListCtrl* ptrPrgDetail) {
	m_ptrEditor = ptrEditor;
	m_ptrPrgDetail = ptrPrgDetail;
}


bool CmdExecutorCtrl::ExecuteSteps_FromTo(long from, long to) {
	m_Btn_ExecStep->Enable(false);
	m_Btn_ExecAll->Enable(false);

	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

	sCommand vStep;
	wxString CmdStr;			//wxMemoryBuffer
	m_Executor.IsRunning(true);
	for (long i = from; i < to; i++) {
		m_ptrPrgDetail->EnsureVisibleCentered(i);
		::wxYield();

		m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);
		m_Executor.ExecuteStep(vStep);
		if (!m_Executor.IsRunning())
			break;
	}
	m_Executor.IsRunning(false);
	LogMe("Stop Execution --------------------------\n", true);

	m_Btn_ExecStep->Enable(true);
	m_Btn_ExecAll->Enable(true);

	return true;
}

void CmdExecutorCtrl::OnBtnCommands(wxCommandEvent& event) {
	//wxButton* btn = static_cast<wxButton*>(event.GetEventObject());
	switch (event.GetId()) {
		case ID_Btn_ExecStep:
			m_Btn_ExecStep->Enable(false);
			m_Btn_ExecAll->Enable(false);
			//Non dal DB ma dall'editor!!!
			{
				sCommand	s = m_ptrEditor->UI2DBData();
				m_Executor.ExecuteStepSingle(s);
			}
			m_Btn_ExecStep->Enable(true);
			m_Btn_ExecAll->Enable(true);
			break;
		case ID_Btn_ExecAll:
			LogMe(wxString::Format("Execute All from = '%ld/%ld'\n", m_ptrEditor->GetProgId(), m_ptrEditor->GetStepId()), true);
			ExecuteSteps_FromTo(0, m_ptrPrgDetail->GetItemCount());	//
			break;
		case ID_Btn_Panic:
			m_Executor.IsRunning(false);
			::wxYield();
			break;
		default:
			LogMe(wxString::Format("(???) Event=%0X\n", event.GetId()), true);
			break;
	}
}

void CmdExecutorCtrl::OnTimer(wxTimerEvent& ) {
	m_timer.Stop();
	bool isReady = m_Executor.IsWorking();
	if (this->IsEnabled() != isReady) {
		this->Enable(isReady);
		LogMe(isReady ? "Device Connected." : "Device Disconnected.", true);
	}
	m_timer.Start(250);	// Restart timer
}

CmdExecutorCtrl::CmdExecutorCtrl(wxWindow* parent,
	wxWindowID		winid,
	const wxPoint&	pos,
	const wxSize&	size,
	long			style,
	const wxString& name
) : wxPanel(parent, winid, pos, size, style, name) {
	m_Btn_ExecAll	= new wxButton(this, ID_Btn_ExecAll,	_("Exec All"));
	m_Btn_ExecStep	= new wxButton(this, ID_Btn_ExecStep,	_("Exec Step"));
	m_Btn_Panic		= new wxButton(this, ID_Btn_Panic,		_("Panic"));
	{
		wxBitmap bitmap;
		if (bitmap.LoadFile("MYSTEPICO", wxBITMAP_TYPE_PNG_RESOURCE))
			m_Btn_ExecStep->SetBitmap(bitmap, wxLEFT);
	}
	m_Btn_ExecAll->SetBitmap(wxArtProvider::GetIcon(wxART_GO_FORWARD, wxART_BUTTON), wxLEFT);
	m_Btn_Panic->SetBitmap(wxArtProvider::GetIcon(wxART_WARNING, wxART_BUTTON), wxLEFT);

	m_Btn_ExecAll->SetToolTip(_("Execute entire program (all steps)"));
	m_Btn_ExecStep->SetToolTip(_("Execute only the selected record"));
	m_Btn_Panic->SetToolTip(_("STOP ALL"));

	{
		wxBoxSizer* SizButtons = new wxBoxSizer(wxVERTICAL);
			SizButtons->Add(m_Btn_ExecAll, 1, wxALL | wxGROW, 1);
			SizButtons->Add(m_Btn_ExecStep, 1, wxALL | wxGROW, 1);
			SizButtons->Add(m_Btn_Panic, 1, wxALL | wxGROW, 1);

		SetSizer(SizButtons);
		Layout(); 	PostSizeEventToParent();
	}

	this->Bind(wxEVT_TIMER, &CmdExecutorCtrl::OnTimer, this, m_timer.GetId());

	m_timer.Start(100);	// millisecond interval
}

CmdExecutorCtrl::~CmdExecutorCtrl() {
	m_Executor.IsRunning(false);
	m_timer.Stop();
	wxYield();
	hid_exit();	//Avoid Memory Leak about error_buffer
}