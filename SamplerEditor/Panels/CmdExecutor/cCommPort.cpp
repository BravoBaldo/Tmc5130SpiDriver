#include "stdwx.h"
#include "cCommPort.h"

cCommPort::cCommPort() { Init(); }

cCommPort::~cCommPort() { Close(); }

void cCommPort::Init() {
	m_HidInfo.m_Name		= wxT("Sampler");
	m_HidInfo.vendor_id		= 0x6666;
	m_HidInfo.product_id	= 0x0827;
	m_HidInfo.serial_number	= nullptr; // C++11 standard nullptr al posto di NULL
}

void cCommPort::Close() {
	if (m_HidExec.IsOpened()) {
		m_HidExec.Close();
	}
}

bool cCommPort::IsWorking() {
	struct hid_device_info* devs = hid_enumerate(m_HidInfo.vendor_id, m_HidInfo.product_id);
	if (devs) {
		hid_free_enumeration(devs);
		if (!m_HidExec.IsOpened())  m_HidExec.Open(m_HidInfo); // Tenta l'apertura solo se chiuso
	} else {
		if (m_HidExec.IsOpened())   m_HidExec.Close(); // Se rimosso fisicamente, chiude la sessione aperta
	}
	return m_HidExec.IsOpened();
}

bool cCommPort::Write(const sCommand* vStep, size_t length, long timeoutMs) {
	wxStopWatch sw;
	sw.Start(0);
	do {
		if (m_HidExec.Write_NoWait((const unsigned char*)vStep, length) >= 0) {
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

size_t cCommPort::Read(std::vector<uint8_t>& outBuffer, uint8_t chk, long timeoutMs) {
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
		//wxMilliSleep(5); //::wxYield();
	} while (sw.Time() < timeoutMs);
	return 0; // Fail
}

