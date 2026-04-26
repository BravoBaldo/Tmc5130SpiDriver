#include "stdwx.h"
#include "sSampler_Commands.h"

// Funzione che accetta un numero variabile di stringhe
std::vector<wxString> RetArray2(std::initializer_list<wxString> lista) {
	return std::vector<wxString>(lista.begin(), lista.end());	// Inizializza il vector direttamente con il contenuto della lista
}

static const sSubSystem SubSystems[]{
	{'A', "ADC Converter"},
	{'B', "BarCode"},
	{'S', "StripLEDS"},
	{'P', "Power Reader"},
	{'M', "Steppers"},
	{'U', "Unused"},
};

unsigned int SubSystem_Size(void) { return WXSIZEOF(SubSystems); };

const sSubSystem*	SubSystem_Get(unsigned int i)	{
	if (i >= WXSIZEOF(SubSystems))
		i = 0;
	return &SubSystems[i];
};
const sSubSystem* SubSystem_GetByType(const char Type) {
	for (size_t i = 0; i < WXSIZEOF(SubSystems); i++) {
		if (SubSystems[i].Type == Type)
			return &SubSystems[i];
	}
	return nullptr;
}

static const sParams SamplerParams[]{
	//ParId	ParType	ParName			MinValue	MaxValue			ToolTip						ParValues;
	{ 'a', eNumber,	"Accel.",		10,				200																				},	//wxUINT16_MAX = 0xffff
	{ 'b', eChoice,	"Boolean",		0,				1,				"True or False"				, RetArray2({"False", "True"})								},
	{ 'c', eNumber,	"Char",			(-128),			127																				},
	{ 'e', eChoice,	"Abilitation",	0,				1,				"Enable or Disable"			, RetArray2({"Disable", "Enable"})							},
	{ 'o', eChoice,	"Open/Close",	0,				1,				"Open/Close"				, RetArray2({"Open", "Close"})								},
	{ 's', eNumber,	"Speed",		500,			25000																			},
	{ 't',   eTime,	"Time",			0,				wxINT32_MAX																		},	//wxUINT32_MAX = 0xffffffff

	{ 'A', eNumber,	"Acc.n",		wxINT32_MIN,	wxINT32_MAX,	"Acceleration"				, RetArray2({"Acceleration"})},
	{ 'C', eNumber,	"Byte",			0,				0xFF																			},
	{ 'M', eChoice,	"Motor",		0,				3,				"0=X, 1=Y, 2=Z"				, RetArray2({"X (Left/Right)", "Y (Up/Down)", "Z (Rotation)", "Probe"})},
	{ 'O', eChoice,	"On/Off",		0,				1,				"Off/ON"					, RetArray2({"Off", "ON"})			},
	{ 'S', eNumber,	"Steps.",		wxINT32_MIN,	wxINT32_MAX,	"Steps"						, RetArray2({"Steps"})},
	{ 'V', eNumber,	"Vel.",			wxINT32_MIN,	wxINT32_MAX,	"Velocity"					, RetArray2({"Velocity"})},
	{ 'i', eNumber,	"Current",		0,				31,				"Current31"},
	{ 'j', eNumber,	"Current",		0,				15,				"Current15"},
	{ 'p', eNumber,	"Process",		0,				 5,				"Laser Game"},
};

static const sSampler_Commands Sampler_Commands[] = {
	// Sub  cmd  Descr				ParamPattern	ParNames										ExtDescr
	  {'M', 'x', "Demo1",			"MbeoMO",		RetArray2({"Stepper", "t_False/True","t_Disable/Enable","t_Open/Close", "t_X/Y/Z/P","t_Off/On"})},
	  {'M', 'y', "Demo2",			"MCsatM",		RetArray2({"Stepper", "t_byte","t_Speed","t_Accel", "t_Time","t_Motor"})},
	  {'M', 'y', "Demo3",			"MSVA",			RetArray2({"Stepper", "t_Step","t_Vel","t_Acc.n"})},
	  {'M', 'w', "Wait End Of",		"MMt",			RetArray2({"Stepper", "Motor", "delay"})},
	  {'M', 'C', "Set Current",		"Miij",			RetArray2({"Stepper", "IHOLD", "IRUN", "IHOLDDELAY"})},
	  {'M', 'E', "GoEnd",			"M",			RetArray2({"Stepper"})},
	  {'M', 'E', "GoEnd 1",			"MV",			RetArray2({"Stepper", "Velocity"})},
	  {'M', 'G', "Goto 1",			"MS",			RetArray2({"Stepper", "Steps"})},
	  {'M', 'G', "Goto 3",			"MAVS",			RetArray2({"Stepper", "Acceleration", "Velocity", "Steps"})},
	  {'M', 'H', "Home",			"M",			RetArray2({"Stepper"})},
	  {'M', 'H', "Home 1",			"MV",			RetArray2({"Stepper", "Velocity"})},
	  {'M', 'T', "Set Trapezoidal",	"MAc",			RetArray2({"Stepper", "Acceleration", "Velocity"})},
	  {'A', 'a', "GetADC",			"",				RetArray2({})},
	  {'B', 'l', "Laser ON",		"O",			RetArray2({"Laser On/Off"})},
	  {'B', 'R', "Read List",		"",				RetArray2({})},
	  {'S', 'l', "Show",			"O",			RetArray2({"Strip On/Off"})},
	  {'S', 'P', "Start Process",	"p",			RetArray2({"Light Game"})},
	  {'P', 'v', "Read Volt",		"",				RetArray2({})},
	  {'P', 'A', "Read Amp",		"",				RetArray2({})},
	  {'P', 'W', "Read Watt",		"",				RetArray2({})},
};

void sSampler_Check(void) {
	//Non ci devono essere
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		const char	ToTest_SubSys	= Sampler_Commands[i].SubSys;
		const char	ToTest_Cmd		= Sampler_Commands[i].cmd;
		unsigned int ToTest_PatLen	= strlen(Sampler_Commands[i].ParamPattern);

		if (ToTest_PatLen > 10 )	//WXSIZEOF(CmdParLabel) defined in CmdEditorCtrl.h
			wxMessageBox(wxString::Format("Too many parameters for ['%c' %d] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);

		if(ToTest_PatLen!= Sampler_Commands[i].ParNames.size())
			wxMessageBox(wxString::Format("Not enaugh names for ['%c' %d] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);

		for (size_t j = i+1; j < WXSIZEOF(Sampler_Commands); j++) {
			if (   ToTest_Cmd == Sampler_Commands[j].cmd
				&& ToTest_PatLen == strlen(Sampler_Commands[j].ParamPattern)
				&& ToTest_SubSys == Sampler_Commands[j].SubSys
			) {
				wxMessageBox(wxString::Format("Duplicated command ['%c' %d] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);
			}
		}
	}
}
unsigned int Commands_Size(void) { return WXSIZEOF(Sampler_Commands); };

const sSampler_Commands* Command_Get(unsigned int i) {
	if (i >= WXSIZEOF(Sampler_Commands))
		i = 0;
	return &Sampler_Commands[i];
}

const sSampler_Commands* Command_GetByCmd(char SubSys, char c, uint8_t NumPar) {
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		if (Sampler_Commands[i].SubSys == SubSys && Sampler_Commands[i].cmd == c && NumPar == strlen(Sampler_Commands[i].ParamPattern))
			return &Sampler_Commands[i];
	}
	return nullptr;
}

/*int Command_GetIdOfCmd(char SubSys, char c, uint8_t NumPar) {
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		if (Sampler_Commands[i].SubSys == SubSys && Sampler_Commands[i].cmd == c && NumPar == strlen(Sampler_Commands[i].ParamPattern))
			return i;
	}
	return -1;
}*/

const sParams* Param_Get(byte Id) {
	for (int i = 0; i < WXSIZEOF(SamplerParams); i++) {
		if(SamplerParams[i].ParId == Id)
			return &SamplerParams[i];
	}
	return nullptr;
}

void Params_RemoveAll(void) {
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		Sampler_Commands[i].ParNames.~vector();;
	}
	for (int i = 0; i < WXSIZEOF(SamplerParams); i++) {
		SamplerParams[i].ParValues.~vector();
	}
}
