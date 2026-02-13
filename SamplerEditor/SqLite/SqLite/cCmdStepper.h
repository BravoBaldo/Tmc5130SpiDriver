#pragma once
#include "wx/wx.h"

class cCmdStepper {
public:
	long		m_MasterId;
	long		m_DetailProg;
	byte		m_Motor;
	byte		m_Cmd;
	wxString	m_Pattern;
	byte		m_Cnt;
	long		m_Par[7];

	void Set(long masterId, long detailProg, byte motor, byte cmd, const wxString& pattern, byte cnt
		, const long p1 = 0, const long p2 = 0, const long p3 = 0, const long p4 = 0, const long p5 = 0, const long p6 = 0, const long p7 = 0) {


		m_MasterId = masterId;
		m_DetailProg = detailProg;
		m_Motor = motor;
		m_Cmd = cmd;
		m_Pattern = pattern;
		m_Cnt = cnt;
		long params[] = { p1,p2,p3,p4,p5,p6,p7 };
		for (int i = 0; i < 7; ++i) m_Par[i] = params[i];
	}

	cCmdStepper(long masterId, long detailProg, byte motor, byte cmd, const wxString& pattern, byte cnt
		, const long p1 = 0, const long p2 = 0, const long p3 = 0, const long p4 = 0, const long p5 = 0, const long p6 = 0, const long p7 = 0) {
		Set(masterId, detailProg, motor, cmd, pattern, cnt, p1, p2, p3, p4, p5, p6, p7);
	};

	cCmdStepper(long masterId, long detailProg, unsigned char motor, unsigned char cmd,
		const wxString& pattern, unsigned char cnt, const long* params = nullptr)
	{
		m_MasterId = masterId;
		m_DetailProg = detailProg;
		m_Motor = motor;
		m_Cmd = cmd;
		m_Pattern = pattern;
		m_Cnt = cnt;

		if (params) {
			for (int i = 0; i < 7; ++i) m_Par[i] = params[i];
		}
		else {
			for (int i = 0; i < 7; ++i) m_Par[i] = 0;
		}
	}

	cCmdStepper() : cCmdStepper(0, 0, 0, 0, wxEmptyString, 0, nullptr) {}

};
