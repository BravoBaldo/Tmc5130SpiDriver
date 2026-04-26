#pragma once
#include "wx/wx.h"
#define NUMOFPARAMS 10
typedef wxArrayInt ParamsType;
//typedef wxArrayLong ParamsType;

class cCmdStepper {
public:
	byte		m_SubSystem;
	byte		m_Cmd;
	wxString	m_Pattern;
	long		m_Par[NUMOFPARAMS];		//ToDo To Remove
	ParamsType	m_ParArr;

	uint16_t	m_MasterId;
	uint16_t	m_DetailProg;

	cCmdStepper(uint16_t masterId, uint16_t detailProg
		, byte				SubSystem
		, byte				cmd
		, const wxString&	pattern
		, const ParamsType	params
	) {
		m_SubSystem		= SubSystem;
		m_MasterId		= masterId;
		m_DetailProg	= detailProg;
		m_Cmd			= cmd;
		m_Pattern		= pattern;
		size_t cnt = wxMin(params.GetCount(), NUMOFPARAMS);
		m_ParArr.Clear();
		for (size_t i = 0; i < cnt; ++i) {
			m_ParArr.Add(params[i]);
			m_Par[i] = params[i];		//ToDo To Remove
		}
	};

	cCmdStepper() { ParamsType T; cCmdStepper(0, 0, 0, 0, wxEmptyString, T); }
};
