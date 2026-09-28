#include "stdwx.h"
#include "wx/artprov.h"
#include <chrono>
#include <wx/stopwatch.h>
#include <winsock2.h>
#include <numeric>	//std::accumulate
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

void myMilliSleep(long long T){
	auto inizio = std::chrono::steady_clock::now();
	std::chrono::milliseconds durata(T); // Imposta la durata qui
	while (std::chrono::steady_clock::now() - inizio < durata){
		wxYield();
	}
}

void CmdExecutorCtrl::SendCommand(const sCommand& vStep, size_t length, long TimeoutMs) {
	bool		Success		= false;
	int			retryCount	= 0;
	wxStopWatch	swTotCmd;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	std::vector<uint8_t> responseBuffer;
	swTotCmd.Start(0);
	while (!Success && m_Running) {

		if (!m_CommPort.Write(&vStep, length, 1000)) {
			LogMe("Hardware error or timeout while writing.\n", true);
			m_Running = false; // Interrompiamo l'esecuzione in caso di guasto hardware persistente
			break;
		}
		LogMe(wxString::Format(" %d ", ++retryCount), false);
		::wxYield();

		size_t bytesRead = m_CommPort.Read(responseBuffer, vStep.m_Cmd, TimeoutMs);
		const AnswerHeader* ptrHeader = reinterpret_cast<const AnswerHeader*>(responseBuffer.data());

		if (bytesRead >= sizeof(AnswerHeader)) {
			if (m_ptrAnswerShow)
				m_ptrAnswerShow->SetAnswer(ptrHeader, bytesRead);

			Success = (ptrHeader->m_Result == eCmdOk);
			if (vStep.m_Cmd != ptrHeader->m_Cmd) {
				LogMe(wxString::Format("\tAAA: Answer non coherent %d != %d\n", (int)vStep.m_Cmd, (int)ptrHeader->m_Cmd), false);
				Success = false;
			}
			LogMe(wxString::Format(" %s ...", Success ? "True\n" : "False"), false);
		} else {
			LogMe(wxString::Format("Timeout scaduto (%ld ms). Ritrasmetto...\n", TimeoutMs), true);
			if (retryCount > 50) {	// Opzionale: aggiungi un limite massimo di tentativi per evitare loop infiniti
				LogMe("\n******* Too many failed attempts. Operation aborted. ****\n\n", true);
				m_Running = false;	//Stop Execution
				break;
			}
		}
	}
	LogMe(wxString::Format("  Completed in %ld ms.\n", swTotCmd.Time()), false);
}

/*
// Versione ottimizzata
uint16_t CalcCheckSum(const uint8_t msg[], size_t len) {
	uint32_t sum = 0; // Usa uint32_t per evitare overflow durante il calcolo
	for (size_t i = 0; i < len; ++i) {
		sum += msg[i];
	}
	// Ritorna il complemento a due della somma troncata a 16 bit
	return (uint16_t)(~sum + 1);
}*/

uint16_t CalcCheckSum(const uint8_t msg[], size_t len) {
	if (msg == nullptr) return 0;
	uint16_t sum = std::accumulate(msg, msg + len, (uint16_t)0);
	return ~sum + 1;
}

/*uint8_t xor_checksum(const uint8_t data[], size_t len) {
	uint8_t checksum = 0;
	for (size_t i = 0; i < len; ++i) {
		checksum ^= data[i];
	}
	return checksum;
}*/

// XOR Checksum moderno
uint8_t xor_checksum(const uint8_t data[], size_t len) {
	if (data == nullptr) return 0;
	return std::accumulate(data, data + len, (uint8_t)0, std::bit_xor<uint8_t>());
}

uint16_t add_checksum_fast(const uint8_t* data, size_t len) {
	uint32_t sum = 0; // Usiamo 32 bit per evitare overflow intermedi nel loop
	for (size_t i = 0; i < len; ++i) {
		sum += data[i];
	}
	return (uint16_t)(~sum + 1);
}

bool CmdExecutorCtrl::ExecuteStep(sCommand& vStep) {
	LogMe("\n\n", false);
	LogMe(wxString::Format("Step %d (%d)\n", vStep.m_DetailProg, vStep.m_Cmd), true);

	//Check SubRoutine:
	if (vStep.m_SubSystem == eSystemCmd && (vStep.m_Cmd=='a')) {
		LogMe(wxString::Format("Execute Subroutine %d\n", vStep.m_Par[0]), false);
		ExecuteSteps(vStep.m_Par[0]);
		return true;
	}

	for (size_t i = 0; i < vStep.m_PatLen; ++i) {		
		if (vStep.m_Pattern[i] == 'S') {
			int	iVal = vStep.m_Par[i];
			if (iVal >= 50000) {
				cDBSampler yy(SQLLITEDBPATH);
				vStep.m_Par[i] = yy.Defaults_NazSteps(iVal - 50000);
			}

		}
	}

	vStep.m_ChkSum = add_checksum_fast(
		reinterpret_cast<const uint8_t*>(&vStep),
		sizeof(vStep) - sizeof(vStep.m_ChkSum)
	);

	SendCommand(vStep, sizeof(vStep));
	
	return true;
}

bool CmdExecutorCtrl::ExecuteSteps(uint16_t	m_MasterId) {	//Execute Steps from DB
	int64_t detailProg = 0;
	sCommand vStep;
	bool recordFound;

#if defined(USE_ODBC)
#else
	{
		cDBSampler yy(SQLLITEDBPATH);
		do {
			recordFound = yy.ProgDetail_Select(m_MasterId, detailProg, vStep);
			if (recordFound) {
				ExecuteStep(vStep);
				detailProg = vStep.m_DetailProg + 1;
			}
		} while (recordFound && m_Running);
	}
#endif

	return true;
}


bool CmdExecutorCtrl::ExecuteSteps(long from, long to) {
m_Btn_ExecStep->Enable(false);
m_Btn_ExecAll->Enable(false);
	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

	sCommand vStep;
	wxString CmdStr;			//wxMemoryBuffer
	m_Running = true;
	for (long i = from; i < to; i++) {
		//m_ptrPrgDetail->Select(i, true);	//Select instruction on the display and get MasterId/
		m_ptrPrgDetail->EnsureVisibleCentered(i);
		::wxYield();

		m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);
		ExecuteStep(vStep);
//		myMilliSleep(10);	//wxMilliSleep(100);
		if (!m_Running)
			break;
	}
	m_Running = false;
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
				m_Running = true;
				sCommand	s = m_ptrEditor->UI2DBData();
				ExecuteStep(s);
				m_Running = false;
			}
			m_Btn_ExecStep->Enable(true);
			m_Btn_ExecAll->Enable(true);
			break;
		case ID_Btn_ExecAll:
			LogMe(wxString::Format("Execute All from = '%ld/%ld'\n", m_ptrEditor->GetProgId(), m_ptrEditor->GetStepId()), true);
			ExecuteSteps(0, m_ptrPrgDetail->GetItemCount());
			break;
		case ID_Btn_Panic:
			m_Running = false;
			::wxYield();
			break;
		default:
			LogMe(wxString::Format("(???) Event=%0X\n", event.GetId()), true);
			break;
	}
}

void CmdExecutorCtrl::OnTimer(wxTimerEvent& ) {
	m_timer->Stop();

	bool isReady = m_CommPort.IsWorking();
	if (this->IsEnabled() != isReady) {
		this->Enable(isReady);
		LogMe(isReady ? "Device Connected." : "Device Disconnected.", true);
	}

	if (isReady && m_PoolMotors && !m_Running) {
		if (m_RotatePool) {
			IncPoolIdx();
		}
		sCommand AskMotor;// = { eTypCommand, eStepDirect, '0', 1, "M", {m_PoolIdx}, 0, 0, 0 };
			AskMotor.m_MsgType		= eTypCommand;
			AskMotor.m_SubSystem	= eStepDirect;
			AskMotor.m_Cmd			= '0';
			AskMotor.m_PatLen		= 1;
			AskMotor.m_Pattern[0]	= 'M';
			AskMotor.m_Par[0]		= m_PoolIdx;
			AskMotor.m_MasterId		= 0;			//2
			AskMotor.m_DetailProg	= 0;			//2
			AskMotor.m_ChkSum		= 0;			//2

		m_Running = true;
		ExecuteStep(AskMotor);
		m_Running = false;
		::wxYield();
	}

	m_timer->Start(250);	// Restart timer
}


CmdExecutorCtrl::CmdExecutorCtrl(wxWindow* parent,
	wxWindowID		winid,
	const wxPoint& pos,
	const wxSize& size,
	long			style,
	const wxString& name
) : wxPanel(parent, winid, pos, size, style, name), m_CommPort()
{
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

		SetSizer(SizButtons);
		Layout(); 	PostSizeEventToParent();
	}

	m_timer = new wxTimer(this, ID_Exec_Timer);
	m_timer->Start(100);	// millisecond interval

}

CmdExecutorCtrl::~CmdExecutorCtrl() {
	m_Running = false;
	m_timer->Stop();	wxYield();	wxDELETE(m_timer);

	hid_exit();	//Avoid Memory Leak about error_buffer
}