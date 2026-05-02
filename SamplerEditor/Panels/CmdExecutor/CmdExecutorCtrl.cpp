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

eCmdAnswer CmdExecutorCtrl::ParseAnswer(const sCommAnsw& Answ) {
	LogMe(wxString::Format("\tSystem    %c\n", Answ.m_SubSystem), true);
	LogMe(wxString::Format("\tRisultato %s\n", (Answ.m_Result == eCmdOk) ? "Ok" : "Ko"), true);
	LogMe(wxString::Format("\tValore    %ld\n", Answ.m_Val), true);
	return Answ.m_Result;
}

//ToDo: Separate Tx and Rx each with own TimeOut

void CmdExecutorCtrl::SendCommand(const unsigned char* data, size_t length, long TimeoutMs) {
	bool Success	= false;
	int res			= 0;
	int retryCount	= 0;
	wxStopWatch sw2;
	if (TimeoutMs <= 0) TimeoutMs = 500;	//Minimal TimeOut

	while (!Success && m_Running) {
		// 1. Transmission
		if (m_HidExec.Write_NoWait(data, length) < 0) {
			LogMe("Errore hardware in scrittura. Apro e Riprovo...\n", true);
			m_HidExec.Open(m_HidInfo);
			wxMilliSleep(100); // Piccola pausa prima di riprovare
			continue;
		}
		LogMe(wxString::Format("Tentativo %d: Messaggio inviato...\n", ++retryCount), true);

		// 2. Attesa risposta con Timeout
		wxStopWatch sw;
		res = 0;

		// Continua a leggere fino a che non ricevi dati O scade il timeout O il programma si ferma
		while (res <= 0 && sw.Time() < TimeoutMs && m_Running) {
			res = m_HidExec.Read(); // Nota: assicurati che Read() sia non-bloccante o abbia un timeout interno breve
			if (res <= 0) {
				wxYield(); // Lascia respirare la UI di wxWidgets
			}
		}

		// 3. Verifica esito
		if (res > 0) {
			Success = true;
			char Tipo = ((char*)m_HidExec.GetBuffer())[0];
			switch (Tipo) {
				case 'a':
					{
						sCommAnsw Risposta;
						memcpy(&Risposta, (sCommAnsw*)m_HidExec.GetBuffer(), sizeof(sCommAnsw));
						LogMe(wxString::Format("Ricevuti %d byte in %ld ms.\n", res, sw.Time()), true);
						if (ParseAnswer(Risposta) != eCmdOk)
							Success = false;
					}
					break;
				default:
					LogMe(wxString::Format("\nERRORE: Risposta non riconosciuta ('%c').\n", Tipo), true);
					//Inutile continuare!
					break;
			}
		} else {
			LogMe(wxString::Format("Timeout scaduto (%ld ms). Ritrasmetto...\n", TimeoutMs), true);
			if (retryCount > 10) {	// Opzionale: aggiungi un limite massimo di tentativi per evitare loop infiniti
				LogMe("Troppi tentativi falliti. Operazione interrotta.\n", true);
				break;
			}
		}
	}
	LogMe(wxString::Format("Concluso in %ld ms.\n", sw2.Time()), true);
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

#define TX_BINARY_M

bool CmdExecutorCtrl::ExecuteStep(cCmdStepper& vStep) {
	unsigned char Msg[sizeof(cCmdStepper) + 1];	// + Starting byte
#if defined(TX_BINARY_M)	//ToDo Check exported data
	int j = 0;
	vStep.m_CheckSum = add_checksum_fast((const uint8_t*)&vStep, sizeof(cCmdStepper) - sizeof(vStep.m_CheckSum));
	memcpy(&Msg[j], &vStep, sizeof(vStep));
	j += sizeof(vStep);
	SendCommand( Msg, sizeof(vStep) );
#else
#endif
	return true;
}

bool CmdExecutorCtrl::ExecuteSteps(long from, long to) {
m_Btn_ExecStep->Enable(false);
m_Btn_ExecAll->Enable(false);
	LogMe(wxString::Format("Start Execution from %ld'\n-----------------------------\n", from), false);

	cCmdStepper vStep;
	wxString CmdStr;			//wxMemoryBuffer
	m_Running = true;
	for (long i = from; i < to; i++) {
		m_ptrPrgDetail->Select(i, true);
		wxYield();
#if defined(TX_BINARY_M)	//ToDo Check exported data
		m_ptrPrgDetail->PrgDetail_FillListItem(vStep, i);
		ExecuteStep(vStep);
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
	m_Running = false;
	LogMe("Stop Execution --------------------------\n", false);
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
				cCmdStepper	s = m_ptrEditor->UI2DBData();
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
			wxYield();
			break;
		default:
			LogMe(wxString::Format("(???) Event=%0X\n", event.GetId()), true);
			break;
	}
}

void CmdExecutorCtrl::OnTimer(wxTimerEvent& ) {
	m_timer->Stop();

	// 1. Verifica presenza fisica del dispositivo
	struct hid_device_info* devs = hid_enumerate(m_HidInfo.vendor_id, m_HidInfo.product_id);

	bool isPresent = (devs != nullptr);
	if (devs) hid_free_enumeration(devs);
	//hid_exit();	//Avoid Memory Leak about error_buffer

	// 2. Gestione connessione
	if (isPresent) {
		if (!m_HidExec.IsOpened()) m_HidExec.Open(m_HidInfo);	// Tenta l'apertura solo se non aperto
	} else {
		if (m_HidExec.IsOpened()) m_HidExec.Close();	// Se non presente ma era aperto, chiudilo pulitamente
	}

	bool isReady = isPresent && m_HidExec.IsOpened();
	// Set UI Status
	if (this->IsEnabled() != isReady) {
		this->Enable(isReady);
		LogMe(isReady ? "Dispositivo Connesso." : "Dispositivo Disconnesso.", true);	// Logga il cambio di stato per debug
	}

	m_timer->Start(250);	// Restart timer
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
	hid_exit();	//Avoid Memory Leak about error_buffer
}