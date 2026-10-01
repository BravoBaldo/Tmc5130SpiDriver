#include "stdwx.h"
#include "wx/artprov.h"
#include "CmdExecutorCtrl.h"

enum {
	ID_Btn_ExecAll = wxID_HIGHEST,
	ID_Btn_ExecEditor,
	ID_Btn_Panic,
};

void CmdExecutorCtrl::SetEditorAndDB(CmdEditorCtrl* ptrEditor, cDetailListCtrl* ptrPrgDetail) {
	m_ptrEditor = ptrEditor;
	m_ptrPrgDetail = ptrPrgDetail;
}

void CmdExecutorCtrl::ShowCurrentStep(long i, sCommand& vStep) {
	if (!m_ptrPrgDetail) return; //
	m_ptrPrgDetail->EnsureVisibleCentered(i);
	::wxSafeYield(this, true);	// ::wxYield();
	m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);	//Read from List
}

bool CmdExecutorCtrl::ExecuteSteps_FromTo(long from, long to) {
	m_Btn_ExecEditor->Enable(false);
	m_Btn_ExecAll->Enable(false);

	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

	m_Executor.IsRunning(true);
	for (long i = from; i < to; i++) {
		sCommand vStep;
		ShowCurrentStep(i, vStep);
		m_Executor.ExecuteStep(vStep);
		if (!m_Executor.IsRunning())
			break;
	}
	m_Executor.IsRunning(false);
	LogMe("Stop Execution --------------------------\n", true);

	m_Btn_ExecEditor->Enable(true);
	m_Btn_ExecAll->Enable(true);

	return true;
}

void CmdExecutorCtrl::OnBtnCommands(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_Btn_ExecEditor:
			m_Btn_ExecEditor->Enable(false);
			m_Btn_ExecAll->Enable(false);
			if (m_ptrEditor) {
				sCommand vStep = m_ptrEditor->UI2DBData();	//Non dal DB ma dall'editor!!!
				m_Executor.ExecuteStepSingle(vStep);
			}
			m_Btn_ExecEditor->Enable(true);
			m_Btn_ExecAll->Enable(true);
			break;
		case ID_Btn_ExecAll:
			if (m_ptrPrgDetail)
			ExecuteSteps_FromTo(0, m_ptrPrgDetail->GetItemCount());	//
			break;
		case ID_Btn_Panic:
			m_Executor.IsRunning(false);
			::wxSafeYield(this, true);	// ::wxYield();
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
) : wxPanel(parent, winid, pos, size, style, name), m_timer(this) {
	m_Btn_ExecAll	= new wxButton(this, ID_Btn_ExecAll,	_("Exec All"));
	m_Btn_ExecEditor= new wxButton(this, ID_Btn_ExecEditor, _("Exec Editor"));
	m_Btn_Panic		= new wxButton(this, ID_Btn_Panic,		_("Panic"));
	{
		wxBitmap bitmap;
		if (bitmap.LoadFile("MYSTEPICO", wxBITMAP_TYPE_PNG_RESOURCE))
			m_Btn_ExecEditor->SetBitmap(bitmap, wxLEFT);
	}
	m_Btn_ExecAll->SetBitmap(wxArtProvider::GetIcon(wxART_GO_FORWARD, wxART_BUTTON), wxLEFT);
	m_Btn_Panic->SetBitmap(wxArtProvider::GetIcon(wxART_WARNING, wxART_BUTTON), wxLEFT);

	m_Btn_ExecAll->SetToolTip(_("Execute entire program (all steps)"));
	m_Btn_ExecEditor->SetToolTip(_("Execute only the selected record"));
	m_Btn_Panic->SetToolTip(_("STOP ALL"));

		wxBoxSizer* SizButtons = new wxBoxSizer(wxVERTICAL);
			SizButtons->Add(m_Btn_ExecAll, 1, wxALL | wxGROW, 1);
		SizButtons->Add(m_Btn_ExecEditor,	1, wxALL | wxGROW, 1);
			SizButtons->Add(m_Btn_Panic, 1, wxALL | wxGROW, 1);

		SetSizer(SizButtons);
	Layout();
	PostSizeEventToParent();

	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands, this, ID_Btn_ExecAll);
	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands, this, ID_Btn_ExecEditor);
	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands, this, ID_Btn_Panic);
	this->Bind(wxEVT_TIMER, &CmdExecutorCtrl::OnTimer, this, m_timer.GetId());

	m_timer.Start(100);	// millisecond interval
}

CmdExecutorCtrl::~CmdExecutorCtrl() {
	m_Executor.IsRunning(false);
	m_timer.Stop();
	::wxSafeYield(this, true);	// wxYield();
	hid_exit();	//Avoid Memory Leak about error_buffer
}