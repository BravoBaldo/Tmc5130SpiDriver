#pragma once

#define USE_INA260        //Current Power, Require USE_SPI
//#define USE_ESPNOW        //486689 bytes
//#define USE_STRIPLED    //365675 bytes
#define USE_HID_SAMPLER
#define USE_EXPANDERS
#define USE_SPI
#define USE_TMC5130       //Require USE_EXPANDERS and USE_SPI
#define USE_TMC5130_FSA   //Require USE_EXPANDERS and USE_SPI
//#define USE_STEPPERS    //Require USE_EXPANDERS and USE_SPI
//#define USE_TMC_Multi_FSA //Require USE_EXPANDERS, USE_SPI, USE_TMC5130, USE_TMC5130_FSA



#define SHOW_DEBUGS
#define ADD_CMD_DESCRIPTION
#define NUMOFPARAMS 10
typedef int32_t		ParamType;	//See in stdwx

typedef enum : uint8_t { eCmdRetry, eCmdOk, eCmdError }eCmdAnswer;

inline constexpr uint8_t CollectorMAC[] = {0x26, 0x0B, 0xA1, 0x4F, 0x92, 0xC3};

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
    X(eStep_UpDwn,	 6,  7, "Motor A: Left/Right") \
    X(eStep_LR,		 4,  5, "Motor B: Up/Dn") \
    X(eStep_Syringe, 2,  3, "Motor C: Rotating Arm")

/*
    X(eStep_Deposit,12, 13, "Motor D: Depositor") \
    X(eStep_Needle,	10, 11, "Motor E") \
    X(eStep_Spare,	 8,  9, "Motor F") \
    X(eStep_TOP,	 0,  0, "") 
*/


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


enum {
#define X(eMotorId, csPin, cePin, description) _dummy_##eMotorId,
	STEPPERS_LIST
#undef X
	NUMBER_OF_MOTORS
};

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
	eTypAnswFsaSingle	= 'f',	//FSA Single Stepper
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

#define FSA_ANSWERS_LIST \
	X(eFsaShowCurrents,		false,	"Currents") \
	X(eFsaShowVel,			true,	"Velocity") \
	X(eFsaTarget,			false,	"Target") \
	X(eFsaShowPos,			true,	"Position") \
	X(eFsaStatus,			false,	"FSA Status") \
	X(eFsaShowCount,		true,	"--")

typedef enum : uint8_t {
#define X(eParamId, eIsAlign, eDescription) eParamId,
	FSA_ANSWERS_LIST
#undef X
}eFsaShowAnswer;




#define NEWCODE
#if defined(NEWCODE)
	#pragma pack(push, 1)
		struct AnswerHeader {
			eMessageTypes   m_MsgType;
			byte			m_Cmd = 0;
			eCmdAnswer		m_Result = eCmdError;

			AnswerHeader(eMessageTypes type) : m_MsgType(type) {}	//Ctor
		};
		struct sAnswerVersion : public AnswerHeader {
			sAnswerVersion() : AnswerHeader(eTypAnswVer) {}

			byte    Y;  // Year
			byte    M;  // Month
			byte    D;  // Day
			byte    h;  // Hour
			byte    m;  // Minute
			byte    s;  // Second
		};
		struct sAnswerPower : public AnswerHeader {
			sAnswerPower() : AnswerHeader(eTypAnswPwReader) {}
			float   Curr;
			float   Volt;
			float   Power;
		};
		struct sAnswerStandard : public AnswerHeader {
			sAnswerStandard() : AnswerHeader(eTypAnswStd) {}
			eSubSysAcro		m_SubSystem		= eUnused;		//1
			eMessageTypes	m_UnknownMsg	= eTypCommand;	//1
			byte			m_AnswLen		= 0;			//1
			char			m_Msg[40]		= "No Answer";	//	
		};
		struct sExpanderStandard : public AnswerHeader {
			sExpanderStandard() : AnswerHeader(eTypAnswExpander) {}
			uint16_t	m_CurrStatus = 0;				//1
		};

		struct StripAnswer : public AnswerHeader {
			StripAnswer() : AnswerHeader(eTypAnswStripLed) {}
			uint8_t		m_CurrGame = 0;	//ToDo
			uint16_t	m_Remaining = 0;
		};
		struct FsaSingleAnswer : public AnswerHeader {
			FsaSingleAnswer() : AnswerHeader(eTypAnswFsaSingle) {}
			uint8_t		m_Motor = 0;
			uint8_t		m_FsaStatus = 0;

			int16_t		m_VACTUAL = 0;	//see m_Velocity  23 bits
			int32_t		m_Position = 0;
			int32_t		m_xTarget = 0;
			uint16_t	m_Currents = 0;	//irun, ihold, holdDelay;
#if defined(USE_INA260)	
			float		m_Curr = 0.;
			float		m_Volt = 0.;
			float		m_Power = 0.;
#endif
		};

		struct TmcAnswer : public AnswerHeader {
			TmcAnswer() : AnswerHeader(eTypAnswStepDir) {}
			uint8_t		m_Motor = 0;

			uint16_t	m_Remaining = 0;
			uint8_t		m_spiStatus = 0;
			uint8_t		m_Ioin8 = 0;
			int32_t		m_Position = 0;
			int32_t		m_xTarget = 0;
			uint16_t	m_Currents = 0;		//irun, ihold, holdDelay;
#if defined(X_SHOW_CHOPCONF)
			uint32_t	m_CHOPCONF = 0;		//Chopconf		getMicrosteps
#endif
			uint32_t	m_DRV_STATUS = 0;	//DrvStatus  getDrvStatus
			uint32_t	m_MSCURACT = 0;

			uint16_t	m_A1 = 0;	//16 bits
			uint16_t	m_AMAX = 0;	//16 bits
			uint16_t	m_DMAX = 0;	//16 bits
			uint16_t	m_D1 = 0;	//16 bits

			uint16_t	m_VSTART = 0;	//18 bits limited to 16
			uint16_t	m_V1 = 0;	//20 bits limited to 16
			uint16_t	m_VMAX = 0;	//23 bits limited to 16
			uint16_t	m_VSTOP = 0;	//18 bits limited to 16
			int16_t	m_VACTUAL = 0;	//see m_Velocity  23 bits
#if defined(SHOW_GCONF)
			uint16_t	m_GCONF = 0;	//18 bits !!! missing direct_mode and test_mode
#endif
#if defined(SHOW_SWMODE)
			uint16_t	m_SWMODE = 0;	//12 bits
#endif
#if defined(USE_INA260)	
			float		m_Curr = 0.;
			float		m_Volt = 0.;
			float		m_Power = 0.;
#endif
		};

	#pragma pack(pop)
#else


#define ANSWERHEADER(T)		byte		m_MsgType	= T;	\
							byte		m_Cmd		= 0;	\
							eCmdAnswer	m_Result	= eCmdError;



#pragma pack(push, 1)
typedef struct _sVerAnswer{
	ANSWERHEADER(eTypAnswVer)
	byte	Y;	//Year
	byte	M;	//Month
	byte	D;	//Day
	byte	h;	//Hour
	byte	m;	//Minute
	byte	s;	//Second
}sAnswerVersion;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sPwrAnswer {
	ANSWERHEADER(eTypAnswPwReader)
	float	Curr;
	float	Volt;
	float	Power;
}sAnswerPower;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sStdAnswer{
	ANSWERHEADER(eTypAnswStd)
	
	eSubSysAcro		m_SubSystem			= eUnused;		//1
	eMessageTypes	m_UnknownMsg		= eTypCommand;	//1
	byte			m_AnswLen			= 0;			//1
	char			m_Msg[40]			= "No Answer";	//	
}sAnswerStandard;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sExpAnswer{
	ANSWERHEADER(eTypAnswExpander)
	uint16_t	m_CurrStatus			= 0;				//1
}sExpanderStandard;	//ToDo Rename in Aswer....
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sStripAnswer{
	ANSWERHEADER(eTypAnswStripLed)
	uint8_t		m_CurrGame	= 0;	//ToDo
	uint16_t	m_Remaining	= 0;
}StripAnswer;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct _sFsaSingleAnswer{	//see STEP_ANSWERS_LIST
	ANSWERHEADER(eTypAnswFsaSingle)	//m_MsgType, m_Cmd, m_Result
	uint8_t		m_Motor		= 0;
	uint8_t		m_FsaStatus	= 0;
	
	int16_t		m_VACTUAL	= 0;	//see m_Velocity  23 bits
	int32_t		m_Position	= 0;
	int32_t		m_xTarget	= 0;
	uint16_t	m_Currents	= 0;	//irun, ihold, holdDelay;
#if defined(USE_INA260)	
	float		m_Curr	= 0.;
	float		m_Volt	= 0.;
	float		m_Power	= 0.;
#endif
}FsaSingleAnswer;
#pragma pack(pop)
	
#pragma pack(push, 1)
typedef struct _sTmcAnswer{	//see STEP_ANSWERS_LIST
	ANSWERHEADER(eTypAnswStepDir)

	uint8_t		m_Motor		= 0;
	
	uint16_t	m_Remaining	= 0;
	uint8_t		m_spiStatus	= 0;
	uint8_t		m_Ioin8		= 0;
	int32_t		m_Position	= 0;
	int32_t		m_xTarget	= 0;
	uint16_t	m_Currents	= 0;		//irun, ihold, holdDelay;
#if defined(X_SHOW_CHOPCONF)
	uint32_t	m_CHOPCONF	= 0;		//Chopconf		getMicrosteps
#endif
	uint32_t	m_DRV_STATUS= 0;	//DrvStatus  getDrvStatus
	uint32_t	m_MSCURACT	= 0;
	
	uint16_t	m_A1		= 0;	//16 bits
	uint16_t	m_AMAX		= 0;	//16 bits
	uint16_t	m_DMAX		= 0;	//16 bits
	uint16_t	m_D1		= 0;	//16 bits
	
	uint16_t	m_VSTART	= 0;	//18 bits limited to 16
	uint16_t	m_V1		= 0;	//20 bits limited to 16
	uint16_t	m_VMAX		= 0;	//23 bits limited to 16
	uint16_t	m_VSTOP		= 0;	//18 bits limited to 16
	 int16_t	m_VACTUAL	= 0;	//see m_Velocity  23 bits
#if defined(SHOW_GCONF)
	uint16_t	m_GCONF		= 0;	//18 bits !!! missing direct_mode and test_mode
#endif
#if defined(SHOW_SWMODE)
	uint16_t	m_SWMODE	= 0;	//12 bits
#endif
#if defined(USE_INA260)	
	float		m_Curr	= 0.;
	float		m_Volt	= 0.;
	float		m_Power	= 0.;
#endif
}TmcAnswer;
#pragma pack(pop)

#endif

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