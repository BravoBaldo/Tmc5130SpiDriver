#pragma once
#include "wx/wx.h"

#include "cHIDAPI.h"
#include "CmdEditorCtrl.h"
#include "DBCmdView.h"
#include "cShowAnswers.h"
#include <wx/stopwatch.h>


class cCommPort {
private:
    sVID_PID m_HidInfo;
    cHIDAPI  m_HidExec;

public:
    // Inizializzazione pulita tramite costruttore
    cCommPort() { Init(); }

    ~cCommPort() { Close(); }

    void Init() {
        m_HidInfo.m_Name        = wxT("Sampler");
        m_HidInfo.vendor_id     = 0x6666;
        m_HidInfo.product_id    = 0x0827;
        m_HidInfo.serial_number = nullptr; // C++11 standard nullptr al posto di NULL
    }

    void Close() {
        if (m_HidExec.IsOpened()) {
            m_HidExec.Close();
        }
    }

    bool IsWorking() {
        struct hid_device_info* devs = hid_enumerate(m_HidInfo.vendor_id, m_HidInfo.product_id);
        if (devs) {
            hid_free_enumeration(devs);
            if (!m_HidExec.IsOpened())  m_HidExec.Open(m_HidInfo); // Tenta l'apertura solo se chiuso
        } else {
            if (m_HidExec.IsOpened())   m_HidExec.Close(); // Se rimosso fisicamente, chiude la sessione aperta
        }
        return m_HidExec.IsOpened();
    }

    bool Write(const unsigned char* data, size_t length, long timeoutMs) {
        wxStopWatch sw;
        sw.Start(0);
        do {
            if (m_HidExec.Write_NoWait(data, length) >= 0) {
                return true; // Success
            }

            // Tentativo di ripristino in caso di errore hardware
            m_HidExec.Open(m_HidInfo);
            wxMilliSleep(50); // A short time

            // NOTA DI SICUREZZA: Evita ::wxYield() se puoi, congela la GUI ma previene crash da re-entry.
            // Se questa funzione viene chiamata dal thread principale della GUI, il ciclo bloccherà l'interfaccia.

        } while (sw.Time() < timeoutMs);

        return false; // Failed
    }

    size_t Read(std::vector<uint8_t>& outBuffer, uint8_t chk, long timeoutMs) {
        outBuffer.clear();  // Svuota il buffer di output prima di iniziare

        wxStopWatch sw;
        sw.Start(0);
        do {
            size_t res = m_HidExec.Read();

            if (res >= sizeof(AnswerHeader)) {
                const AnswerHeader* ptrAnswer = reinterpret_cast<const AnswerHeader*>(m_HidExec.GetBuffer());
                if (ptrAnswer && chk == ptrAnswer->m_Cmd) {
                    const uint8_t* rawBuffer = reinterpret_cast<const uint8_t*>(m_HidExec.GetBuffer());
                    outBuffer.assign(rawBuffer, rawBuffer + res);
                    return res;
                }
            }
            ::wxYield();
        } while (sw.Time() < timeoutMs);
        return 0; // Fail
    }
};


class CmdExecutorCtrl : public wxPanel {
	wxButton*	m_Btn_ExecAll	= nullptr;
	wxButton*	m_Btn_ExecStep	= nullptr;
	wxButton*	m_Btn_Panic		= nullptr;
	wxTimer*	m_timer			= nullptr;

    cCommPort   m_CommPort;

	bool		m_Running		= false;

	ParamType	m_PoolIdx		= 0;
	bool		m_PoolMotors	= false;
	bool		m_RotatePool	= true;
	//....................................
	CmdEditorCtrl*		m_ptrEditor		= nullptr;
	cDetailListCtrl*	m_ptrPrgDetail	= nullptr;
	cAnswersShow*		m_ptrAnswerShow	= nullptr;
	//...................................
	DECLARE_EVENT_TABLE()
	void		OnBtnCommands	(wxCommandEvent& Evt);
	void		OnTimer			(wxTimerEvent& Evt);
	bool		ExecuteStep		(sCommand& vStep);
	void		SendCommand		(const unsigned char* data, size_t length, long TimeoutMs = 500);

public:
	CmdExecutorCtrl	(	wxWindow*		parent,
						wxWindowID		winid	= wxID_ANY,
						const wxPoint&	pos		= wxDefaultPosition,
						const wxSize&	size	= wxDefaultSize,
						long			style	= wxTAB_TRAVERSAL | wxNO_BORDER,
						const wxString&	name	= wxPanelNameStr
					);
	~CmdExecutorCtrl();
	void	SetEditorAndDB(CmdEditorCtrl* ptrEditor, cDetailListCtrl* ptrPrgDetail) {
		m_ptrEditor = ptrEditor;
		m_ptrPrgDetail = ptrPrgDetail;
	}
	bool		ExecuteSteps(long from, long to);
	bool		ExecuteSteps(uint16_t	m_MasterId);
	void		SetAnswerHandler(cAnswersShow* phandler)	{ m_ptrAnswerShow = phandler; }

	void		SetPoolMotors	(bool s, bool r = true)		{ m_PoolMotors = s; m_RotatePool = r; }
	void		IncPoolIdx		(void)						{ m_PoolIdx = (m_PoolIdx + 1) % 3; }
	void		SetPoolIdx		(int idx)					{ m_PoolIdx = idx; }

	int			GetMotorSelected(void) { return m_ptrAnswerShow->GetMotorSelected(); }
};