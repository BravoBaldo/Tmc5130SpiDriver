#include "stdwx.h"
#include "SamplerCmd.h"



static const struct sParams {
	byte			ParId;
	byte			ParSize;
	wxString		ParName;
	long			MinValue;
	long			MaxValue;
	wxString		ToolTip;
	wxArrayString	ParNames;
}SamplerParams[]{
	{ 'b',	sizeof(byte),		"Boolean",			0,				1							, "True or False"					, RetArray("False", "True")								},
	{ 'e',	sizeof(byte),		"Abilitation",		0,				1							, "Enable or Disable"				, RetArray("Disable", "Enable")							},
	{ 'o',	sizeof(byte),		"Open/Close",		0,				1							, "Open/Close"						, RetArray("Open", "Close")								},
	{ 'O',	sizeof(byte),		"On/Off",			0,				1							, "Off/ON"							, RetArray("Off", "ON")									},
	{ 'c',	sizeof(char),		"Char",				(-128),			127																	},
	{ 'C',	sizeof(byte),		"Byte",				0,				0xFF																},
	{ 's',	sizeof(wxUint16),	"Speed",			500,			25000														},
	{ 'a',	sizeof(wxUint16),	"Accel.",			10,				200															},	//wxUINT16_MAX = 0xffff
	{ 't',	sizeof(wxUint32),	"Time",				0,				wxINT32_MAX													},	//wxUINT32_MAX = 0xffffffff
	{ 'M',	sizeof(byte),		"Motor",			0,				3,							"0=X, 1=Y, 2=Z"				, RetArray("X (Left/Right)", "Y (Up/Down)", "Z (Rotation)", "Probe")},
	{ 'S',	sizeof(int32_t),	"Steps",			wxINT32_MIN,	wxINT32_MAX,				"Steps"						, RetArray("Steps")},
	{ 'V',	sizeof(int32_t),	"Velocity",			wxINT32_MIN,	wxINT32_MAX,				"Velocity"					, RetArray("Velocity")},
	{ 'A',	sizeof(int32_t),	"Acceleration",		wxINT32_MIN,	wxINT32_MAX,				"Acceleration"				, RetArray("Acceleration")},
};

static const struct {
	const char		cmd;
	int				cnt;
	const char		*Descr;
	const char		*ParamPattern;
	wxArrayString	ParNames;
	wxString		ExtDescr;
}Sampler_Commands[] = {
// cmd  cnt Descr				ParamPattern	ParNames										ExtDescr
  {'G', 1,  "Goto 1",			"S",			RetArray("Steps")},
  {'G', 3,  "Goto 3",			"AVS",			RetArray("Acceleration", "Velocity", "Steps")},
  {'T', 2,  "Set Trapezoidal",	"AV",			RetArray("Acceleration", "Velocity")},
  {'H', 0,  "Home",				"",				RetArray("")},
  {'H', 1,  "Home 1",			"V",			RetArray("Velocity")},
  {'E', 0,  "GoEnd",			"",				RetArray("")},
  {'E', 1,  "GoEnd 1",			"",				RetArray("Velocity")},
  //  {'E', 2,  "GoEnd"   },
//  {'C', 6,  "Current" },
};


static const struct {
	const byte  id;
	int cnt;
	const char* Description;
}SamplerParam[] = {
  {'G', 6,  "Goto"    },
  {'H', 2,  "Home"    },
  {'E', 2,  "GoEnd"   },
  {'C', 6,  "Current" },
};


/*
GoTo			Steps
Goto			Acc, Speed, Steps
SetTrapezoidal	Amax, Dmax,Vel
StopMotor		
StopMotor		Accel
Current			Curr_irun, Curr_ihold, Curr_holdDelay
GoHome	
HomeParameters	run_current, hold_current, target_position, velocity, acceleration, zero_wait_duration
Sethold_delay     hold_delay
SetIhold          ihold
SetIrun           irun
setNeutral			--
setParking			--
setMicrosteps		MicroStep
setMotorDirection	Direction

*/

static const struct {
	const char  cmd;
	int cnt;
	const char* Description;
}ParsedParams[] = {
  {'G', 6,  "Goto"    },
  {'H', 2,  "Home"    },
  {'E', 2,  "GoEnd"   },
  {'C', 6,  "Current" },
};


typedef struct {
	byte Motor;
	byte Cmd;
	byte Len;
	int  Params[5];
}sSamplerCmd;



void Commands_Fill(wxChoice* Cho) {
	if (Cho) {
		for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
			Cho->Append(Sampler_Commands[i].Descr, (void*)&Sampler_Commands[i]);
		}
	}
}
