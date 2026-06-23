#pragma once

#define SHOW_DEBUGS
#define NUMOFPARAMS 10
typedef int32_t		ParamType;	//See in stdwx

typedef enum : uint8_t { eCmdRetry, eCmdOk, eCmdError }eCmdAnswer;

#define SUBSYS_LIST \
    X(eSystemCmd,    'Q', "System") \
    X(eADCConverter, 'A', "ADC Converter") \
    X(eBarCode,      'K', "BarCode") \
    X(eStripLed,     'S', "StripLEDS") \
    X(ePwReader,     'P', "Power Reader") \
    X(eExpanders,    'E', "Expanders") \
    X(eSteppersFSA,  'M', "Steppers FSA") \
    X(eStepDirect,   'D', "SteppersDirect") \
    X(eStepNoMotor,	 'F', "Stepper No Motor") \
    X(eUnused,       'U', "Unused")

#define STEPPERS_LIST \
    X(eStep_UpDwn,	 6,  7, "Motor A: Up/Dn") \
    X(eStep_LR,		 4,  5, "Motor B: Left/Right") \
    X(eStep_Syringe, 2,  3, "Motor C: Syringe/Diluter") \
    X(eStep_Deposit,12, 13, "Motor D: Depositor") \
    X(eStep_Needle,	10, 11, "Motor E") \
    X(eStep_Spare,	 8,  9, "Motor F") \
    X(eStep_TOP,	 0,  0, "") 


//		Id, RunAlwais, Name
#define STRIPLEDGAMES_LIST \
    X(eNone,				true,	"None"					) \
    X(eFixedItalianFlagL,	false,	"FixedItalianFlag_Sx"	) \
    X(eFixedItalianFlagR,	false,	"FixedItalianFlag_Dx"	) \
    X(eMoveSingle_Sx,		true,	"MoveSingle_Sx"			) \
    X(eMoveSingle_Dx,		true,	"MoveSingle_Dx"			) \
    X(eMoveDouble_Sx,		true,	"MoveDouble_Sx"			) \
    X(eMoveDouble_Dx,		true,	"MoveDouble_Dx"			) \
    X(eMoveFlagItaly_Sx,	true,	"MoveFlagItaly_Sx"		) \
    X(eMoveFlagItaly_Dx,	true,	"MoveFlagItaly_Dx"		) \
    X(eGraysSx,				false,	"GraysSx"				) \
    X(eGraysDx,				false,	"GraysDx"				) \
    X(eMoveArrow_Sx,		true,	"MoveArrow_Sx"			) \
    X(eMoveArrow_Dx,		true,	"MoveArrow_Dx"			) \
    X(eFade,				true,	"Fade"					) \
    X(eBouncing,			true,	"Bouncing"				) \
    X(eShowNum,				true,	"Show Number"			)

typedef enum : uint8_t {
#define X(eStripId, eRunAlways, description) eStripId,
	STRIPLEDGAMES_LIST
#undef X
}StripGame;

typedef struct {
  StripGame		GameId;
  bool			RunAlways;
  const char*	Description;
}sStripLedGames;


enum eStep_List : uint8_t {
#define X(eMotorId, csPin, cePin, description) eMotorId,
	STEPPERS_LIST
#undef X
};


enum eSubSysAcro : uint8_t {
#define X(acronym, character, description) acronym = character,
	SUBSYS_LIST
#undef X
};

typedef enum : uint8_t {	// AnswerType is lowercase
	eTypCommand			= 'b',	//sCommand
	eTypAnswVer			= 'v',	//sAnswerVersion
	eTypAnswStd			= 'u',	//sAnswerStandard
	eTypAnswConverter	= 'a',	//
	eTypAnswBarCode		= 'k',	//
	eTypAnswStripLed	= 's',	//StripAnswer
	eTypAnswPwReader	= 'p',
	eTypAnswExpander	= 'e',	//sExpanderStandard
	eTypAnswStepDir		= 'd',	//TmcAnswer
//	eTypStepperRegs		= 'r',	//StepperRegsAnswer
}eMessageTypes;


#pragma pack(push, 1)
typedef struct _sCmd{	//Command from PC ToDo: See class cCmdStepper
	eMessageTypes	m_MsgType				= eTypCommand;	//1
	eSubSysAcro		m_SubSystem				= eUnused;		//1		//Come from sSampler_Commands
	byte			m_Cmd					= 0;			//1		//Come from sSampler_Commands
	byte			m_PatLen				= 0;			//1
	byte			m_Pattern[NUMOFPARAMS]	= {'\0'};		//10	//Come from sSampler_Commands
	ParamType  		m_Par[NUMOFPARAMS]		= {0};			//4*10	//Come from sSampler_Commands
	uint16_t		m_MasterId				= 0;			//2
	uint16_t		m_DetailProg			= 0;			//2
	uint16_t  		m_ChkSum				= 0;			//2
	//inline const char* GetPatternAsChars(void) const { return (char*)m_Pattern; }
	inline const char* GetPatternAsChars() const noexcept {
		return reinterpret_cast<const char*>(m_Pattern);
	}
	void	SetPattern(const char* s) {
		m_PatLen = s ? strlen(s) : 0;
		for (byte i = 0; i < NUMOFPARAMS; ++i) {
			m_Pattern[i] = (i < m_PatLen) ? s[i] : '\0';
		}
	}
}sCommand;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sVerAnswer{
	byte	m_MsgType			= eTypAnswVer;	//1
	byte	Y;
	byte	M;
	byte	D;
	byte	h;
	byte	m;
	byte	s;
}sAnswerVersion;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct _sStdAnswer{
	byte			m_MsgType			= eTypAnswStd;	//1
	eSubSysAcro		m_SubSystem			= eUnused;		//1
	byte			m_Cmd				= 0;			//1
	eMessageTypes	m_UnknownMsg		= eTypCommand;	//1
	eCmdAnswer		m_Result			= eCmdOk;
	byte			m_AnswLen			= 0;			//1
	char			m_Msg[40]			= "No Answer";	//	
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




#define SHOW_SWMODE
#define SHOW_SWMODE_HIDELATCH
//#define SHOW_GCONF
//#define SHOW_CHOPCONF

#ifdef SHOW_SWMODE
    #define X_SHOW_SWMODE X(eStpShowSWMODE, true, "SWMODE")
#else
    #define X_SHOW_SWMODE
#endif

#ifdef SHOW_GCONF
    #define X_SHOW_GCONF X(eStpShowGCONF, true, "GCONF")
#else
    #define X_SHOW_GCONF
#endif

#ifdef SHOW_CHOPCONF
    #define X_SHOW_CHOPCONF X(eStpShowChopConf, true, "ChopConf")
#else
    #define X_SHOW_CHOPCONF
#endif

#define STEP_ANSWERS_LIST \
	X_SHOW_SWMODE \
	X(eStpShowSpiStatus,	false,	"Status") \
	X(eStpShowIoin8,		true,	"Ioin") \
	X(eStpShowVel,			true,	"Velocities") \
	X(eStpShowAccels,		true,	"Accelerations") \
	X(eStpShowPos,			true,	"Positions") \
	X(eStpShowCurrents,		false,	"Currents") \
	X_SHOW_CHOPCONF \
	X(eStpShowDrvStatus,	true,	"DrvStatus") \
	X(eStpShowMsCurAct,		true,	"MSCURACT") \
	X_SHOW_GCONF \
	X(eStpShowTime,			false,	"Time") \
	X(eStpShowCount,		true,	"--")

typedef enum : uint8_t {
#define X(eParamId, eIsAlign, eDescription) eParamId,
	STEP_ANSWERS_LIST
#undef X
}eStepShowAnswer;


#pragma pack(push, 1)
typedef struct _sTmcAnswer{	//see STEP_ANSWERS_LIST
	byte		m_MsgType	= eTypAnswStepDir;	//1
	eCmdAnswer	m_Result	= eCmdOk;

	uint8_t		m_Motor;
	
	uint16_t	m_Remaining	= 0;
	uint8_t		m_spiStatus;
	uint8_t		m_Ioin8;
	int32_t  	m_Position;
	int32_t		m_xTarget;
	uint16_t	m_Currents;		//irun, ihold, holdDelay;
#if defined(X_SHOW_CHOPCONF)
	uint32_t	m_CHOPCONF;		//Chopconf		getMicrosteps
#endif
	uint32_t	m_DRV_STATUS;	//DrvStatus  getDrvStatus
	uint32_t	m_MSCURACT;
	
	uint16_t	m_A1;		//16 bits
	uint16_t	m_AMAX;		//16 bits
	uint16_t	m_DMAX;		//16 bits
	uint16_t	m_D1;		//16 bits
	
	uint16_t	m_VSTART;	//18 bits limited to 16
	uint16_t	m_V1;		//20 bits limited to 16
	uint16_t	m_VMAX;		//23 bits limited to 16
	uint16_t	m_VSTOP;	//18 bits limited to 16
	 int16_t	m_VACTUAL;	//see m_Velocity  23 bits
#if defined(SHOW_GCONF)
	uint16_t	m_GCONF		= 0;	//18 bits !!! missing direct_mode and test_mode
#endif
#if defined(SHOW_SWMODE)
	uint16_t	m_SWMODE	= 0;	//12 bits
#endif
	//ChipEnabled
}TmcAnswer;
#pragma pack(pop)

static_assert(sizeof(TmcAnswer) <= 64, "Error: TmcAnswer exceeds the maximum size of 64 bytes!");

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