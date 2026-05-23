#pragma once

#define SHOW_DEBUGS
#define NUMOFPARAMS 10
typedef int32_t		ParamType;	//See in stdwx

//
typedef enum : uint8_t { eCmdRetry, eCmdOk, eCmdError }eCmdAnswer;
/*
	c'è differenza tra m_MsgType e m_SubSystem
*/

typedef enum : uint8_t {	//System in Uppercase
	eSystemCmd		= 'Q',
	eADCConverter	= 'A',
	eBarCode		= 'K',
	eStripLed		= 'S',
	ePwReader		= 'P',
	eExpanders		= 'E',
	eSteppers		= 'M',
	eStepDirect		= 'D',
	eUnused			= 'U',
}eSubSysAcro;	// See sSampler_Commands.cpp


typedef enum : uint8_t {	// AnswerType is lowercase
	eTypCommand			= 'b',
	eTypAnswStd			= 'u',
	eTypAnswConverter	= 'a',
	eTypAnswBarCode		= 'k',	
	eTypAnswStripLed	= 's',
	eTypAnswPwReader	= 'p',
	eTypAnswExpander	= 'e',
	eTypAnswStepper		= 'm',	
	eTypAnswStepDir		= 'd',
	eTypStepperRegs		= 'r',
}eMessageTypes;


#pragma pack(push, 1)
typedef struct _sCmd{	//Command from PC ToDo: See class cCmdStepper
	eMessageTypes	m_MsgType				= eTypCommand;	//1
	eSubSysAcro		m_SubSystem				= eUnused;		//1
	byte			m_Cmd					= 0;			//1
	byte			m_PatLen				= 0;			//1
	byte			m_Pattern[NUMOFPARAMS]	= {'\0'};		//10
	ParamType  		m_Par[NUMOFPARAMS]		= {0};			//4*10
	uint16_t		m_MasterId				= 0;			//2
	uint16_t		m_DetailProg			= 0;			//2
	uint16_t  		m_ChkSum				= 0;			//2
	inline const char* GetPatternAsChars(void) const { return (char*)m_Pattern; }
	void	SetPattern(const char* s) {
		m_PatLen = s ? strlen(s) : 0;
		for (byte i = 0; i < NUMOFPARAMS; ++i) {
			m_Pattern[i] = (i < m_PatLen) ? s[i] : '\0';
		}
	}
}sCommand;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sStdAnswer{
	byte			m_MsgType				= eTypAnswStd;	//1
	eSubSysAcro		m_SubSystem				= eUnused;		//1
	byte			m_Cmd					= 0;			//1
	eMessageTypes	m_UnknownMsg			= eTypCommand;	//1
	eCmdAnswer		m_Result				= eCmdOk;
	byte			m_AnswLen				= 0;			//1
	char			m_Msg[40]				= "No Answer";	//	
}sAnswerStandard;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sExpAnswer{
	byte		m_MsgType				= eTypAnswExpander;	//1
	uint16_t	m_CurrStatus			= 0;				//1
}sExpanderStandard;	//ToDo Rename in Aswer....
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sStripAnswer{
	byte		m_MsgType	= eTypAnswStripLed;	//1
	uint8_t		m_CurrGame	= 0;	//ToDo
	uint16_t	m_Remaining	= 0;
	eCmdAnswer	m_Result	= eCmdOk;
}StripAnswer;
#pragma pack(pop)

typedef enum : uint8_t	{	eStpShowTime,   eStpShowSpiStatus, eStpShowIoin8,    eStpShowVel, eStpShowPos, 
							eStpShowTarget, eStpShowCurrents,  eStpShowChopConf, eStpShowDrvStatus, eStpShowMsCurAct,
							eStpShowCount
						}eStepShowAnswer;
                         
#pragma pack(push, 1)
typedef struct _sTmcAnswer{
	byte		m_MsgType	= eTypAnswStepDir;	//1
	eCmdAnswer	m_Result	= eCmdOk;

	uint8_t		m_Motor;
	
	uint16_t	m_Remaining	= 0;
	uint8_t		m_spiStatus;
	uint8_t		m_Ioin8;
	uint32_t	m_Velocity;
	int32_t  	m_Position;
	int32_t		m_xTarget;
	uint16_t	m_Currents;		//irun, ihold, holdDelay;
	uint32_t	m_CHOPCONF;		//Chopconf		getMicrosteps
	uint32_t	m_DRV_STATUS;	//DrvStatus  getDrvStatus
	uint32_t	m_MSCURACT;
	//ChipEnabled
}TmcAnswer;
#pragma pack(pop)

static_assert(sizeof(TmcAnswer) <= 64, "Error: TmcAnswer exceeds the maximum size of 64 bytes!");

#pragma pack(push, 1)
typedef struct _sStepAnswer{
	byte		m_MsgType		= eTypAnswStepper;	//1
	eSubSysAcro	m_SubSystem		= eSteppers;		//1
	byte		m_Cmd			= 0;				//1
	uint16_t	m_MasterId		= 0;				//2
	uint16_t	m_DetailProg	= 0;				//2
	eCmdAnswer	m_Result		= eCmdOk;			//1

	uint8_t		m_Motor;
//	uint16_t	m_Remaining	= 0;
	uint8_t		m_spiStatus;	//GetSpiStatus().bytes
	uint8_t		m_Ioin8;		//(getIoin().bytes & 0xFF)
	uint32_t	m_Velocity;
	int32_t  	m_Position;
	int32_t		m_xTarget;
	uint8_t		m_irun;
	uint8_t		m_ihold;
	uint8_t		m_holdDelay;
	uint8_t		m_FsaStatus;	//FSA_Status	FSA.Status
}StepperAnswer;	// AAA To Remove see TmcAnswer
#pragma pack(pop)
static_assert(sizeof(StepperAnswer) <= 64, "Error: StepperAnswer exceeds the maximum size of 64 bytes!");

/*
#pragma pack(push, 1)
typedef struct _sStepRegs{
	byte		m_MsgType		= eTypStepperRegs;	//1
	eSubSysAcro	m_SubSystem		= eStepDirect;		//1
	eCmdAnswer	m_Result		= eCmdOk;			//1

	byte		m_Motor			= 0;
	byte		m_RegFrom		= 0;
	byte		m_NumReg		= 0;
	int32_t   	m_Reg[13]		= {0};			//4*10
}StepperRegsAnswer;
#pragma pack(pop)

void ReadRegs(StepperRegsAnswer& S, uint8_t From, uint8_t nToread){	//AAA: nToread<= WXSIZEOF(StepperRegsAnswer.m_Reg)
	for(uint8_t i=0; i<nToread; i++) S.m_Reg[i]=readReg(i+From);
}
*/