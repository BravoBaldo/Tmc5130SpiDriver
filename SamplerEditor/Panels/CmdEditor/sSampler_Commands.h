#pragma once
#include "wx/wx.h"
#include "stdwx.h"

typedef enum { eChoice, eNumber, eTime, eUnknown } eParType;

struct sSubSystem {
	eSubSysAcro		Type;
	wxString		Descr;
};

unsigned int				SubSystem_Size(void);
const sSubSystem*			SubSystem_Get(unsigned int i);
const sSubSystem*			SubSystem_GetByType(const char Type);

typedef struct {	//Commands' parameters
	byte					ParId;
	eParType				ParType;
	wxString				ParName;
	long					MinValue;
	long					MaxValue;
	wxString				ToolTip;
	std::vector<wxString>	ParValues;
}sParams;

const sParams* Param_Get(byte Id);


typedef struct {	//Microcontroller Commands. See 
	const eSubSysAcro		SubSys;
	const char				cmd;
	const char*				Descr;
	const char*				ParamPattern;
	std::vector<wxString>	ParNames;
	wxString				ExtDescr;
	decltype(cmd)			CmdTyp;
}sSampler_Commands;

unsigned int				Commands_Size(void);
const sSampler_Commands*	Command_Get(unsigned int i);
const sSampler_Commands*	Command_GetByCmd(char SubSys, char c, uint8_t NumPar);
void						Params_RemoveAll(void);

void						sSampler_Check(void);
