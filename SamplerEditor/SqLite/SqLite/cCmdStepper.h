#pragma once
#include <wx/wx.h>
#include "stdwx.h"

#pragma pack(push, 1) // Salva l'allineamento attuale e imposta a 1 byte
class cCmdStepper {
public:
	byte		m_MsgType = 'b';		//1
	byte		m_SubSystem;			//1
	byte		m_Cmd;					//1
	byte		m_PatLen;				//1
	byte		m_Pattern[NUMOFPARAMS];	//10
	ParamType	m_Par[NUMOFPARAMS];		//10*sizeof(ParamType)
	uint16_t	m_MasterId;				//2
	uint16_t	m_DetailProg;			//2
	uint16_t	m_CheckSum;				//2

	void	SetPattern(const char* s) { 
					m_PatLen = s ? strlen(s) : 0;
					for (byte i = 0; i < NUMOFPARAMS; ++i) {
						m_Pattern[i] = (i < m_PatLen) ? s[i] : '\0';
					}
				}
	inline const char*	GetPatternAsChars(void) const {	return (char*)m_Pattern; }

	cCmdStepper(uint16_t masterId, uint16_t detailProg
		, byte				SubSystem
		, byte				cmd
		, const byte		PatLen
		, const char*		pattern
		, const ParamType	params[]
	) {
		//m_MsgType	= 'b';
		m_SubSystem		= SubSystem;
		m_MasterId		= masterId;
		m_DetailProg	= detailProg;
		m_Cmd			= cmd;
		SetPattern(pattern);
		size_t cnt = wxMin(PatLen, WXSIZEOF(m_Pattern));
		for (size_t i = 0; i < WXSIZEOF(m_Pattern); ++i) {
			m_Par[i]  = ( (i<cnt)?params[i] : 0);
		}
	};

	cCmdStepper() : cCmdStepper(0, 0, 0, 0, 0, "", nullptr){}
};
#pragma pack(pop) // Ripristina l'allineamento originale
