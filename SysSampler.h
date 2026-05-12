#pragma once

#define SHOW_DEBUGS
#define NUMOFPARAMS 10
typedef int32_t		ParamType;	//See in stdwx

//
typedef enum : uint8_t { eCmdOk, eCmdRetry, eCmdError }eCmdAnswer;
/*
	c'è differenza tra m_MsgType e m_SubSystem
*/

typedef enum : uint8_t {	//Command in Uppercase
	eADCConverter	= 'A',
	eBarCode		= 'B',
	eStripLed		= 'S',
	ePwReader		= 'P',
	eSteppers		= 'M',
	eExpanders		= 'E',
	eUnused			= 'U',
}eSubSysAcro;	// See sSampler_Commands.cpp


typedef enum : uint8_t {	// lowercase
	eTypCommand			= 'b',
	eTypAnswStd			= 'x',
	eTypAnswStepper		= 's',
	
	eTypAnswConverter	= 'x',
	eTypAnswBarCode		= 'b',
	eTypAnswStripLed	= 'x',
	eTypAnswPwReader	= 'x',
	eTypAnswExpander	= 'x',
}eMessageTypes;


#pragma pack(push, 1)
typedef struct _sCmd{	//Command from PC
	eMessageTypes	m_MsgType				= eTypCommand;	//1
	eSubSysAcro		m_SubSystem				= eUnused;		//1
	byte			m_Cmd					= 0;			//1
	byte			m_PatLen				= 0;			//1
	byte			m_Pattern[NUMOFPARAMS]	= {'\0'};		//10
	int32_t   		m_Par[NUMOFPARAMS]		= {0};			//4*10
	uint16_t		m_MasterId				= 0;			//2
	uint16_t		m_DetailProg			= 0;			//2
	uint16_t  		m_ChkSum				= 0;			//2
}sCommand;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _sStdAnswer{
	byte			m_MsgType				= eTypAnswStd;			//1
	eSubSysAcro		m_SubSystem				= eUnused;		//1
	byte			m_Cmd					= 0;			//1
	eMessageTypes	m_UnknownMsg			= eTypCommand;	//1
	byte			m_AnswLen				= 0;			//1
	char			m_Msg[40]				= "No Answer";	//	
}sAnswerStandard;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct _sStepAnswer{
	byte		m_MsgType		= eTypAnswStepper;	//1
	eSubSysAcro	m_SubSystem		= eSteppers;		//1
	byte		m_Cmd			= 0;				//1
	uint16_t	m_MasterId		= 0;				//2
	uint16_t	m_DetailProg	= 0;				//2
	eCmdAnswer	m_Result		= eCmdOk;			//1

	uint8_t		m_Motor;
	uint8_t		m_spiStatus;	//GetSpiStatus().bytes
	uint8_t		m_Ioin8;		//(getIoin().bytes & 0xFF)
	uint8_t		m_FsaStatus;	//FSA_Status	FSA.Status
	uint32_t	m_Velocity;
	int32_t  	m_Position;
	int32_t		m_xTarget;
	uint8_t		m_irun;
	uint8_t		m_ihold;
	uint8_t		m_holdDelay;
}StepperAnswer;
#pragma pack(pop)
