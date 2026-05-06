#pragma once

#define NUMOFPARAMS 10
typedef int32_t		ParamType;	//See in stdwx

typedef enum : uint8_t { eCmdOk, eCmdRetry, eCmdError }eCmdAnswer;

typedef enum : uint8_t {
	eADCConverter	= 'A',
	eBarCode		= 'B',
	eStripLed		= 'S',
	ePwReader		= 'P',
	eSteppers		= 'M',
	eExpanders		= 'E',
	eNone			= '0',
}eSubSysAcro;	// See sSampler_Commands.cpp

#pragma pack(push, 1) // Salva l'allineamento attuale e imposta a 1 byte
typedef struct _sStepAnswer{
	byte		m_MsgType = 's';							//1
	eSubSysAcro	m_SubSystem = eSteppers;				//1
	byte		m_Cmd = 0;								//1
	uint16_t	m_MasterId = 0;								//2
	uint16_t	m_DetailProg = 0;								//2
	eCmdAnswer	m_Result = eCmdOk;						//1

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
#pragma pack(pop) // Ripristina l'allineamento originale
