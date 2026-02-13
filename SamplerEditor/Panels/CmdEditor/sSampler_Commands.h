#pragma once
#include "wx/wx.h"

typedef enum { eChoice, eNumber, eTime, eUnknown } eParType;


typedef struct {	//Commands' parameters
	byte					ParId;
	eParType				ParType;
	wxString				ParName;
	long					MinValue;
	long					MaxValue;
	wxString				ToolTip;
	std::vector<wxString>	ParValues;
}sParams;

typedef struct {	//Microcontroller Commands
	const char				cmd;
	const char*				Descr;
	const char*				ParamPattern;
	std::vector<wxString>	ParNames;
	wxString				ExtDescr;
	decltype(cmd)			CmdTyp;
}sSampler_Commands;

typedef struct {	//Microcontroller Commands Identification
	const char	cmd;
	uint8_t		NumPar;
}sCommandId;

unsigned int				Commands_Size(void);
const sSampler_Commands*	Command_Get(unsigned int i);
const sSampler_Commands*	Command_GetByCmd(char c, uint8_t NumPar);
int							Command_GetIdOfCmd(char c, uint8_t NumPar);
const sParams*				Param_Get(byte Id);
void						sSampler_Check(void);
void						Params_RemoveAll(void);

