#include "stdwx.h"
#include "wx/artprov.h"
#include "CmdExecutorCtrl.h"

enum {
	ID_Btn_ExecAll = wxID_HIGHEST,
	ID_Btn_ExecStep,
	ID_Btn_Panic,
	ID_Exec_Timer,
};

BEGIN_EVENT_TABLE(CmdExecutorCtrl, wxPanel)
	EVT_BUTTON(-1, CmdExecutorCtrl::OnBtnCommands)
	EVT_TIMER(ID_Exec_Timer, OnTimer)
END_EVENT_TABLE()

#include <chrono>
void myMilliSleep(long long T){
	auto inizio = std::chrono::steady_clock::now();
	std::chrono::milliseconds durata(T); // Imposta la durata qui
	while (std::chrono::steady_clock::now() - inizio < durata){
		wxYield();
	}
}

bool CmdExecutorCtrl::ExecuteSteps(long from, long to) {
	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

#define USEDIRECTADDRESSING
#if defined(USEDIRECTADDRESSING)
	cCmdStepper vStep;
#endif
	wxString CmdStr;

	for (long i = from; i < to; i++) {
		m_ptrPrgDetail->Select(i, true);
		wxYield();
#if defined(USEDIRECTADDRESSING)
		CmdStr = (m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i)) ? m_ptrEditor->DBData2String(vStep) : "------------";
		LogMe(wxString::Format("Execute %ld: '%s'\n", i, CmdStr), false);
#else

		if (!m_Running) {
			LogMe("EXECUTION INTERRUPTED\n", false);
			return false;
		}
		CmdStr = (m_ptrEditor) ? m_ptrEditor->UI2String() : "Unknown";

		LogMe(wxString::Format("Execute %ld: '%s'\n", i, CmdStr), false);
#endif
//		myMilliSleep(10);	//wxMilliSleep(100);
	}
	LogMe("Stop Execution --------------------------\n", false);
	return true;
}

bool CmdExecutorCtrl::ExecuteFrom(long from, long to) {
	m_Running = true;
	return ExecuteSteps(from, to);
}

void CmdExecutorCtrl::OnBtnCommands(wxCommandEvent& event) {
	//wxButton* btn = static_cast<wxButton*>(event.GetEventObject());
	switch (event.GetId()) {
		case ID_Btn_ExecStep:
			{	//Ask the Editor for the current command and send it

				long itemIndex = m_ptrPrgDetail->GetFirstSelected();
				ExecuteFrom(itemIndex, itemIndex + 1);
			}
			break;
		case ID_Btn_ExecAll:
			{	// Ask for ProgramId then scan all steps
				LogMe(wxString::Format("Execute All from = '%ld/%ld'\n", m_ptrEditor->GetProgId(), m_ptrEditor->GetStepId()), true);
				//Soluzione 1, Senza DB, preleva i dati dalla riga corrente, logga poi passa alla successiva:
				ExecuteFrom(0, m_ptrPrgDetail->GetItemCount());
			}
			break;
		case ID_Btn_Panic:
			m_Running = false;
			wxYield();
			break;
		default:
			LogMe(wxString::Format("(???) Event=%0X\n", event.GetId()), true);
			break;
	}
}

void CmdExecutorCtrl::OnTimer(wxTimerEvent& WXUNUSED(event)) {
	m_timer->Stop();
	//if (m_Running) {
		bool v = m_HidExec.IsOpenable(m_HidInfo);
		if (v && !m_HidExec.IsOpened())	m_HidExec.Open(m_HidInfo);
		this->Enable(v && m_HidExec.IsOpened());
	//}

	m_timer->Start(50);
}

CmdExecutorCtrl::CmdExecutorCtrl(wxWindow* parent,
	wxWindowID		winid,
	const wxPoint& pos,
	const wxSize& size,
	long			style,
	const wxString& name
) : wxPanel(parent, winid, pos, size, style, name)
{
	//Fill Sampler Info
	m_HidInfo.m_Name = "Sampler";
	m_HidInfo.vendor_id = 0x6666;
	m_HidInfo.product_id = 0x0827;
	m_HidInfo.serial_number = NULL;

	m_Btn_ExecAll	= new wxButton(this, ID_Btn_ExecAll, _("Exec All"));
	m_Btn_ExecStep	= new wxButton(this, ID_Btn_ExecStep, _("Exec Step"));
	m_Btn_Panic		= new wxButton(this, ID_Btn_Panic, _("Panic"));
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

		this->SetSizer(SizButtons);
		this->Layout();
	}

	m_timer = new wxTimer(this, ID_Exec_Timer);
	m_timer->Start(100);	// millisecond interval

}

CmdExecutorCtrl::~CmdExecutorCtrl() {
	m_Running = false;
	m_timer->Stop();	wxYield();	wxDELETE(m_timer);

	if (m_HidExec.IsOpened())
		m_HidExec.Close();
}