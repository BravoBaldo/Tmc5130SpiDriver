#include "stdwx.h"
#include "wx/artprov.h"
#include "CmdExecutorCtrl.h"
#include <chrono>
#include <wx/stopwatch.h>
#include <winsock2.h>

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

void myMilliSleep(long long T){
	auto inizio = std::chrono::steady_clock::now();
	std::chrono::milliseconds durata(T); // Imposta la durata qui
	while (std::chrono::steady_clock::now() - inizio < durata){
		wxYield();
	}
}

//wxLongLong
void CmdExecutorCtrl::SendCommand(const unsigned char* data, size_t length, long long /*TimoutMs*/) {
	m_HidExec.Write_NoWait(data, length);	// Writing...
	wxYield();
	LogMe(wxString::Format("Message sent..."), false);

	// Reading...
	wxStopWatch sw;
	int res = 0, cnt=0;
	while(res==0 && m_Running){
		cnt++;
		wxYield();
		res = m_HidExec.Read();
	}
	long DeltaT = sw.Time();
	LogMe(wxString::Format("Received %d bytes in %d and loops %ld ms: '%s'.\n", res, cnt, DeltaT, m_HidExec.GetBuffAsString()), false);
}

uint8_t xor_checksum(const uint8_t* data, size_t len) {
	uint8_t checksum = 0;
	for (size_t i = 0; i < len; ++i) {
		checksum ^= data[i];
	}
	return checksum;
}

#define TX_MODE_M

bool CmdExecutorCtrl::ExecuteSteps(long from, long to) {
	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

	cCmdStepper vStep;
	wxString CmdStr;			//wxMemoryBuffer
	unsigned char Msg[100];
	for (long i = from; i < to; i++) {
		m_ptrPrgDetail->Select(i, true);
		wxYield();
#if defined(TX_MODE_M)
		m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);
		int j = 0;
		Msg[j++] = '0';
		Msg[j++] = vStep.m_Motor;
		Msg[j++] = vStep.m_Cmd;
		vStep.m_Cnt = vStep.m_Pattern.Length();
		Msg[j++] = vStep.m_Cnt;// vStep.m_Cnt;

		memcpy(&Msg[j], vStep.m_Pattern.c_str().AsUnsignedChar(), vStep.m_Cnt);
		j += vStep.m_Cnt;

		std::memcpy(&Msg[j], &vStep.m_Par[0], sizeof(long)*vStep.m_Cnt);
		j += sizeof(long) * vStep.m_Cnt;
		//-----------------------------
		Msg[j] = vStep.m_MasterId;		j += sizeof(vStep.m_MasterId);
		Msg[j] = vStep.m_DetailProg;	j += sizeof(vStep.m_DetailProg);
		Msg[j] = xor_checksum(Msg, j);

		SendCommand(Msg, j+1);	//Append XOR Checksum
#else
		CmdStr = (m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i)) ? m_ptrEditor->DBData2String(vStep) : "------------";
		LogMe(wxString::Format("Execute %ld: '%s'\n", i, CmdStr), false);

		memcpy(Msg, CmdStr.c_str().AsUnsignedChar(), CmdStr.Length());	Msg[CmdStr.Length()] = '\0';
		if (!m_Running) {
			LogMe("EXECUTION INTERRUPTED\n", false);
			return false;
		}
		SendCommand(CmdStr.c_str().AsUnsignedChar(), CmdStr.Length());
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
			m_Btn_ExecStep->Enable(false);
			m_Btn_ExecAll->Enable(false);
			{	//Ask the Editor for the current command and send it
				long itemIndex = m_ptrPrgDetail->GetFirstSelected();
				ExecuteFrom(itemIndex, itemIndex + 1);
			}
			m_Btn_ExecStep->Enable(true);
			m_Btn_ExecAll->Enable(true);
			break;
		case ID_Btn_ExecAll:
			m_Btn_ExecStep->Enable(false);
			m_Btn_ExecAll->Enable(false);
			{	// Ask for ProgramId then scan all steps
				LogMe(wxString::Format("Execute All from = '%ld/%ld'\n", m_ptrEditor->GetProgId(), m_ptrEditor->GetStepId()), true);
				//Soluzione 1, Senza DB, preleva i dati dalla riga corrente, logga poi passa alla successiva:
				ExecuteFrom(0, m_ptrPrgDetail->GetItemCount());
			}
			m_Btn_ExecStep->Enable(true);
			m_Btn_ExecAll->Enable(true);
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