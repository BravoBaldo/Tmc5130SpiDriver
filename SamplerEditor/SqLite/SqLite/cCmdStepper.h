#pragma once
#include <wx/wx.h>
#include "stdwx.h"

#pragma pack(push, 1) //
class cCmdStepper {	//ToDo See struct sCommand
	eMessageTypes	m_MsgType = eTypCommand;			//1
public:
	sCommand		m_command = { eTypCommand };

	eSubSysAcro		m_SubSystem;						//1
	byte			m_Cmd;								//1
	byte			m_PatLen;							//1
	byte			m_Pattern[NUMOFPARAMS] = { '\0' };	//10
	ParamType  		m_Par[NUMOFPARAMS] = { 0 };			//4*10
	uint16_t		m_MasterId;							//2
	uint16_t		m_DetailProg;						//2
	uint16_t		m_CheckSum;							//2

	bool	ComparePassage(void) {
		bool RetVal = true;
		if (m_command.m_MsgType		!= m_MsgType	)	RetVal = false;
		if (m_command.m_SubSystem	!= m_SubSystem	)	RetVal = false;
		if (m_command.m_Cmd			!= m_Cmd		)	RetVal = false;
		if (m_command.m_PatLen		!= m_PatLen		)	RetVal = false;
		if (m_command.m_MasterId	!= m_MasterId	)	RetVal = false;
		if (m_command.m_DetailProg	!= m_DetailProg	)	RetVal = false;
		//if (m_command.m_ChkSum		!= m_CheckSum	)	RetVal = false;
		for (int i = 0; i < NUMOFPARAMS; i++) {
			if (m_command.m_Pattern[i]	!= m_Pattern[i])	RetVal = false;
			if (m_command.m_Par[i]		!= m_Par[i])		RetVal = false;
		}
		if (RetVal)	return true;
		return false;
		//return RetVal;
	}

	/*void	Assign(const sSampler_Commands& C, ParamType Par[NUMOFPARAMS]) {
		m_command.m_SubSystem	= C.SubSys;
		m_command.m_Cmd			= C.cmd;
		SetPattern(C.ParamPattern);
		for (size_t i = 0; i < C.ParNames.size(); i++) {
			m_command.m_Par[i] = Par[i]->GetValue();
		}
	}*/

	void	SetPattern(const char* s) { 
					m_PatLen = s ? strlen(s) : 0;
					for (byte i = 0; i < NUMOFPARAMS; ++i) {
						m_Pattern[i]			= (i < m_PatLen) ? s[i] : '\0';
						m_command.m_Pattern[i]	= (i < m_PatLen) ? s[i] : '\0';
					}
				}
	inline const char*	GetPatternAsChars(void) const {	return (char*)m_Pattern; }
	
	cCmdStepper(const sCommand& Cmd) {
		m_command = Cmd;
		//........................
		m_SubSystem		= Cmd.m_SubSystem;
		m_MasterId		= Cmd.m_MasterId;
		m_DetailProg	= Cmd.m_DetailProg;
		m_Cmd			= Cmd.m_Cmd;
		SetPattern((const char*)Cmd.m_Pattern);
		size_t cnt = wxMin(Cmd.m_PatLen, WXSIZEOF(Cmd.m_Pattern));
		for (size_t i = 0; i < WXSIZEOF(Cmd.m_Pattern); ++i) {
			m_Par[i] = ((i < cnt) ? Cmd.m_Par[i] : 0);
		}
		ComparePassage();
	}
	
	cCmdStepper(  uint16_t			masterId
				, uint16_t			detailProg
				, eSubSysAcro		SubSystem
				, byte				cmd
				, const byte		PatLen
				, const char*		pattern
				, const ParamType	params[]
	) {
		m_command.m_MsgType		= eTypCommand;
		m_command.m_SubSystem	= SubSystem;
		m_command.m_MasterId	= masterId;
		m_command.m_DetailProg	= detailProg;
		m_command.m_Cmd			= cmd;
		SetPattern(pattern);
		size_t cnt = wxMin(PatLen, WXSIZEOF(m_command.m_Pattern));
		for (size_t i = 0; i < WXSIZEOF(m_command.m_Pattern); ++i) {
			m_command.m_Par[i] = ((i < cnt) ? params[i] : 0);
		}
		//...........
		m_MsgType		= eTypCommand;
		m_SubSystem		= SubSystem;
		m_MasterId		= masterId;
		m_DetailProg	= detailProg;
		m_Cmd			= cmd;
		SetPattern(pattern);
		cnt = wxMin(PatLen, WXSIZEOF(m_Pattern));
		for (size_t i = 0; i < WXSIZEOF(m_Pattern); ++i) {
			m_Par[i]  = ( (i<cnt)?params[i] : 0);
		}
		ComparePassage();
	};

//	cCmdStepper() : cCmdStepper(0, 0, eUnused, 0, 0, "", nullptr){ ComparePassage(); }
	cCmdStepper() : cCmdStepper(sCommand()) { ComparePassage(); }
};
#pragma pack(pop) // Ripristina l'allineamento originale
