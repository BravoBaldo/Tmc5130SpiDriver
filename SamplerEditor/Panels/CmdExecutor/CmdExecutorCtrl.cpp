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

void CmdExecutorCtrl::ExecuteProcess(uint16_t m_MasterId, uint16_t from, uint16_t to, std::function<void(sCommand&)> onStepCallback) {
	m_Executor.StartProcess(m_MasterId, from, to, onStepCallback);
}

void CmdExecutorCtrl::ShowCurrentStep(long i, sCommand& vStep) {
	if (!m_ptrPrgDetail) return; //
	m_ptrPrgDetail->EnsureVisibleCentered(i);
	::wxSafeYield(this, true);	// ::wxYield();
	m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);	//Read from List
}


void CmdExecutorCtrl::OnBtnCommands(wxCommandEvent& event) {
	switch (event.GetId()) {
		case ID_Btn_ExecEditor:		//Get sCommand from the Editor
			if (m_ptrEditor) {
				sCommand vStep = m_ptrEditor->UI2DBData();	//Non dal DB ma dall'editor!!!
				m_Executor.ExecuteStepSingle(vStep);
			}
			break;
		case ID_Btn_ExecAll:	//Get vStep from the ListView m_ptrPrgDetail
			{
				sCommand vStep;
				ShowCurrentStep(0, vStep);		//Get vStep from the ListView m_ptrPrgDetail
				ExecuteProcess(vStep.m_MasterId, 0, wxUINT16_MAX, 
					[this](sCommand& v) -> void {
					long n = m_ptrPrgDetail->FindItem(-1, wxString::Format("%d", v.m_DetailProg), false);
					m_ptrPrgDetail->EnsureVisibleCentered(n);
					::wxSafeYield(this, true);
					});
			}
			break;
		case ID_Btn_Panic:
			m_Executor.IsRunningSet(false);
			::wxSafeYield(this, true);
			break;
		default:
			LogMe(wxString::Format("(???) Event=%0X\n", event.GetId()), true);
			break;
	}
}

void CmdExecutorCtrl::OnTimer(wxTimerEvent& ) {
	m_timer.Stop();

	bool IsStartable = !m_Executor.IsRunning();
	m_Btn_ExecEditor->Enable(IsStartable);
	m_Btn_ExecAll	->Enable(IsStartable);

	bool isReady = m_Executor.IsUSBConnected();	//Is USB connected
	if (this->IsEnabled() != isReady) {
		this->Enable(isReady);
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
			SizButtons->Add(m_Btn_ExecAll,		1, wxALL | wxGROW, 1);
			SizButtons->Add(m_Btn_ExecEditor,	1, wxALL | wxGROW, 1);
			SizButtons->Add(m_Btn_Panic,		1, wxALL | wxGROW, 1);

		SetSizer(SizButtons);
	Layout();
	PostSizeEventToParent();

	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands,	this, ID_Btn_ExecAll);
	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands,	this, ID_Btn_ExecEditor);
	this->Bind(wxEVT_BUTTON, &CmdExecutorCtrl::OnBtnCommands,	this, ID_Btn_Panic);
	this->Bind(wxEVT_TIMER,	 &CmdExecutorCtrl::OnTimer,			this, m_timer.GetId());

	m_timer.Start(100);	// millisecond interval

	this->Bind(wxEVT_FSAEXEC_ANSWER,	&CmdExecutorCtrl::OnThreadNotification, this);
	this->Bind(wxEVT_EXEC_PROMPT_USER,	&CmdExecutorCtrl::OnThreadAskUser, this);
	m_Executor.SetEventTarget(this);
}

CmdExecutorCtrl::~CmdExecutorCtrl() {
	m_timer.Stop();
	m_Executor.IsRunningSet(false);
	::wxSafeYield(this, true);	// wxYield();
	hid_exit();	//Avoid Memory Leak about error_buffer
}

void CmdExecutorCtrl::OnThreadAskUser(wxCommandEvent& event) {
	cFsaExec* executor = static_cast<cFsaExec*>(event.GetClientData());	// Recuperiamo il puntatore all'esecutore che ha generato l'evento
	if (!executor) return;

	// Mostriamo il dialogo grafico in modo sicuro sul thread della UI
	wxMessageDialog dialog(this, wxT("Si è verificato un errore di comunicazione. Vuoi continuare comunque il processo?"),
		wxT("Errore Esecuzione"), wxNO_DEFAULT | wxYES_NO | wxICON_ERROR);
	bool userChoice = (dialog.ShowModal() == wxID_YES);
	executor->PostUserResponse(userChoice);		// Rispondiamo asincronamente alla macchina a stati
}

void CmdExecutorCtrl::OnThreadNotification(wxCommandEvent& event) {
	void* rawData = event.GetClientData();
	if (!rawData) return;

	sFsaExecParams* answ = static_cast<sFsaExecParams*>(rawData);
	switch (event.GetId()) {
		case ID_EXEC_ANSWER:
			{
				const auto* ptrHeader = reinterpret_cast<const AnswerHeader*>(answ->Response.data());
				if (m_ptrAnswerShow && ptrHeader)	m_ptrAnswerShow->SetAnswer(ptrHeader, answ->lenAnswer);
			}
			break;
		case ID_EXEC_LOGME:
			LogMe(answ->ToLog, answ->UseDate);
			break;
		default:
			LogMe(
				wxString::Format("OnThreadNotification: ricevuto ID evento non gestito (%d)\n", event.GetId()), true
			);
			break;
	}
	delete answ;
}
