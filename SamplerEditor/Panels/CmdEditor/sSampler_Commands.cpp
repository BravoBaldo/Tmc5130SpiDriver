#include "stdwx.h"
#include "sSampler_Commands.h"

// Funzione che accetta un numero variabile di stringhe
std::vector<wxString> RetArray2(std::initializer_list<wxString> lista) {
	return std::vector<wxString>(lista.begin(), lista.end());	// Inizializza il vector direttamente con il contenuto della lista
}

static const sSubSystem SubSystems[]{
#define X(acronym, character, description) { acronym, description },
	SUBSYS_LIST
#undef X
};

unsigned int SubSystem_Size(void) { return WXSIZEOF(SubSystems); };

const sSubSystem*	SubSystem_GetByIndex(unsigned int i)	{
	return (i < WXSIZEOF(SubSystems)) ? &SubSystems[i] : nullptr;
};

const sSubSystem* SubSystem_GetByType(eSubSysAcro Type) {
	for (size_t i = 0; i < WXSIZEOF(SubSystems); i++) {
		if (SubSystems[i].Type == Type)
			return &SubSystems[i];
	}
	return nullptr;
}

static const sParams SamplerParams[]{
	//ParId	ParType	ParName			MinValue	MaxValue			ToolTip						ParValues;
	{ 'a', eChoice,	"Accelerations",0,				3,				"(Ac)(De)celerations"		, RetArray2({"FirstAcceleration (A1)", "SecondAcceleration (AMAX)", "FirstDeceleration (DMAX)", "SecondDeceleration (D1)"})								},
	{ 'b', eChoice,	"Boolean",		0,				1,				"True or False"				, RetArray2({"False", "TRUE"})								},
	{ 'c', eNumber,	"Char",			(-128),			127																				},
	{ 'd', eChoice,	"Velocities",	0,				3,				"Velocities"				, RetArray2({"VSTART", "V1", "VMAX", "VSTOP"})								},
	{ 'e', eChoice,	"Abilitation",	0,				1,				"Enable or Disable"			, RetArray2({"Disable", "ENABLE"})							},
	{ 'f', eNumber,	"Register",		0,			  115,				"TMC5130 Register"																		},
	{ 'g', eChoice,	"Effect",		0,				0,				"Led Effect"				, RetArray2({
																												#define X(eStripId, eRunAlways, description) description,
																													STRIPLEDGAMES_LIST
																												#undef X
																											})},
	{ 'i', eNumber,	"Current",		0,				31,				"Current31"},
	{ 'j', eNumber,	"Current",		0,				15,				"Current15"},
	{ 'l', eChoice,	"Left/Right",	0,				1,				"Direction"					, RetArray2({"Left", "RIGHT"})								},
	{ 'm', eChoice,	"MicroSteps",	0,				8,				"Microsteps"				, RetArray2({"0=51200=1/256", "1=25600=1/128", "2=12800=1/64", "3=6400=1/32", "4=3200=1/16", "5=1600=1/8", "6=800=1/4", "7=400=1/2", "8=200=1/1"})},
	{ 'n', eNumber,	"ShowedNumber",	0,				6,				"Show Number"},
	{ 'o', eChoice,	"Open/Close",	0,				1,				"Open/Close"				, RetArray2({"Open", "Close"})								},
//	{ 'p', eNumber,	"Process",		0,				5,				"Laser Game"},
	{ 'q', eChoice,	"Left/Right/None",	0,				2,			"Direction"					, RetArray2({"Left", "Right", "None"})},
	{ 's', eNumber,	"Speed",		500,			25000																			},
	{ 't',   eTime,	"Time",			0,				MAX_PARAM																		},	//wxUINT32_MAX = 0xffffffff
//	{ 'u', eChoice, "Wait User",	0,				1,				"0=Go ahead, 1:Wait User"	, RetArray2({"Go ahead", "Wait User"})						},
	{ 'v', eChoice,	"Output",		0,				0,				"Outputs"					, RetArray2({"All/None", "A1 EV Ingresso siringa/diluitore","A2 EV Acqua/Aria in vaso espansione","A3 EV Acqua pozzetto lavaggio ago","P1 Pompa Carico acqua/aria","P2 Pompa Scarico pozzetto lavaggio","P3-AUX"})							},
	{ 'w', eChoice,	"Waitings",		0,				5,				"Waiting Motor"				, RetArray2({"eWaitVelocity", "eWaitPosition", "eWaitHome", "eWaitPosAndVel", "eWaitTimer"})							},


	{ 'A', eNumber,	"Acc.n",		MIN_PARAM,		MAX_PARAM,		"Acceleration"				, RetArray2({"Acceleration"})		},
	{ 'C', eNumber,	"Byte",			0,				0xFF																			},
	{ 'L', eNumber,	"Uint32",		0,				wxINT32_MAX,	"if>50000 ask database"		, RetArray2({"Message"})			},	//wxUINT32_MAX = 0xffffffff
	{ 'M', eChoice,	"Motor",		0,				3,				"0=X, 1=Y, 2=Z"				, RetArray2({"X (Left/Right)", "Y (Up/Down)", "Z (Rotation)", "Probe"})},
	{ 'O', eChoice,	"On/Off",		0,				1,				"Off/ON"					, RetArray2({"Off", "ON"})			},
	{ 'R', eChoice,	"Ramp Mode",	0,				1,				"Ramp Mode"					, RetArray2({"PositionMode", "VelocityPositiveMode", "VelocityNegativeMode", "HoldMode"})},
	{ 'S', eNumber,	"Steps.",		MIN_PARAM,		MAX_PARAM,		"Steps"						, RetArray2({"Steps"})},
	{ 'V', eNumber,	"Vel.",			MIN_PARAM,		MAX_PARAM,		"Velocity"					, RetArray2({"Velocity"})},
	{ 'P', eNumber,	"Routine",		2000,			65535,			"Routine from DB"			, RetArray2({"Routine" })},
};

static const sSampler_Commands Sampler_Commands[] = {
	// Sub  cmd  Descr				ParamPattern	ParNames										ExtDescr

	{eSystemCmd, 'a',	"Exec. Routine",		"P",		RetArray2({"Program Id"					})},
//	{eSystemCmd, 'b',	"Show Message",			"Lu",		RetArray2({"Message Code", "Wait User"	})},
//	{eSystemCmd, 'c',	"Show Image",			"L",		RetArray2({"Image Code"					})},
	{eSystemCmd, '0',	"Get Version",			"",			RetArray2({								})},


	{eStepNoMotor,	'0', "Do Nothing",			"",			RetArray2({											})},
	{eStepNoMotor,	'1', "Set Current Motor",	"M",		RetArray2({"Stepper"								})},
	{eStepNoMotor,	'2', "Set Register",		"fL",		RetArray2({"Register", "Value"					})},

	{eStepNoMotor,	'a', "ChipEnable",			"e",		RetArray2({"Chip Abilitation"				})},

	{eStepNoMotor,	'b', "EndStops NONE (DoNotUse)",	"",			RetArray2({									})},
	{eStepNoMotor,	'b', "EndStops (DoNotUse)",			"q",		RetArray2({"Direction"						})},
	{eStepNoMotor,	'b', "Set EndStops",				"eeeeeee",	RetArray2({"SwapLR", "EnStopL", "EnPoolL", "EnStopR", "EnPoolR", "EnSg", "EnSoft"})},

	{eStepNoMotor,	'c', "Set Currents",		"iij",		RetArray2({"IHOLD", "IRUN", "IHOLDDELAY"	})},
	{eStepNoMotor,	'd', "Set Position",		"S",		RetArray2({"Position"						})},
	{eStepNoMotor,	'e', "Set Microsteps",		"m",		RetArray2({"MicroSteps"						})},
	{eStepNoMotor,	'f', "Set Target",			"S",		RetArray2({"Final Position"					})},

	{eStepNoMotor,	'g', "Set Trapezoidal 2",	"AV",		RetArray2({"Acceleration", "Max Velocity"	})},
	{eStepNoMotor,	'g', "Set SixPoint",		"VAVAVAAV",	RetArray2({"vstart", "a1", "v1", "amax", "vmax", "dmax", "d1", "vstop"	})},
	{eStepNoMotor,	'g', "Set SixPoint Simple",	"VAVAV",	RetArray2({"vstart/vstop", "a1/d1", "v1", "amax/dmax", "vmax"	})},
	{eStepNoMotor,	'g', "Set Trapezoidal 3",	"AVA",		RetArray2({"amax", "vmax", "dmax"	})},

	{eStepNoMotor,	'h', "Set Ramp Mode",		"R",		RetArray2({"Ramp Mode"						})},

	{eStepNoMotor,	'i', "Set Timer",			"t",		RetArray2({"Time"							})},
	{eStepNoMotor,	'j', "Wait",				"wb",		RetArray2({"Wait for", "Check Timer"		})},

	{eStepNoMotor,	'l', "InitGoto",			"VVAAV",	RetArray2({"StartVelocity", "StopVelocity", "FirstAcceleration", "SecondDeceleration", "FirstVelocity"})},
	{eStepNoMotor,	'm', "Set Free Running",	"Vml",		RetArray2({"SpeedFor1RPS", "MicroSteps", "Direction"	})},
	{eStepNoMotor,	'n', "Set Accelerations",	"aA",		RetArray2({"Acc/De-celeration type", "Accel. Value"		})},
	{eStepNoMotor,	'o', "Set Velocities",		"dV",		RetArray2({"Velocity type", "Velocity Value"			})},

	{eStepNoMotor,	'p', "Set Direction",		"l",		RetArray2({"Direction"			})},

	{eStepNoMotor,	'q', "Set GCONF 1",		"bbbbb",		RetArray2({"I_scale_analog", "internal_Rsense", "en_pwm_mode", "enc_commutation", "shaft"					})},
	{eStepNoMotor,	'q', "Set GCONF 2",		"bbbbbbb",		RetArray2({"diag0_error", "diag0_otpw", "diag0_stall", "diag1_stall", "diag1_index", "diag1_onstate", "diag1_steps_skipped"	})},
	{eStepNoMotor,	'q', "Set GCONF 3",		"bbbb",			RetArray2({"small_hysteresis", "stop_enable", "direct_mode", "test_mode"																			})},

	//---------------------------------------------------------------------------------------------------------
	{eStepDirect,	'0', "Do Nothing",			"M",		RetArray2({"Stepper"								})},
	{eStepDirect,	'2', "Set Register",		"MfL",		RetArray2({"Stepper", "Register", "Value"			})},
	{eStepDirect,	'a', "ChipEnable",			"Me",		RetArray2({"Stepper", "Chip Abilitation"			})},

	{eStepDirect,	'b', "EndStops NONE (DoNotUse)",	"M",		RetArray2({"Stepper", 								})},
	{eStepDirect,	'b', "EndStops (DoNotUse)",			"Mq",		RetArray2({"Stepper", "Direction"					})},
	{eStepDirect,	'b', "Set EndStops",					"Meeeeeee",	RetArray2({"Stepper", "SwapLR", "EnStopL", "EnPoolL", "EnStopR", "EnPoolR", "EnSg", "EnSoft"})},

	{eStepDirect,	'c', "Set Currents",		"Miij",		RetArray2({"Stepper", "IHOLD", "IRUN", "IHOLDDELAY"	})},
	{eStepDirect,	'd', "Set Position",		"MS",		RetArray2({"Stepper", "Position"					})},
	{eStepDirect,	'e', "Set Microsteps",		"Mm",		RetArray2({"Stepper", "MicroSteps"					})},
	{eStepDirect,	'f', "Set Target",			"MS",		RetArray2({"Stepper", "Final Position"				})},

	{eStepDirect,	'g', "Set Trapezoidal 2",	"MAV",		RetArray2({"Stepper", "Acceleration", "Max Velocity"})},
	{eStepDirect,	'g', "Set SixPoint",		"MVAVAVAAV",RetArray2({"Stepper", "vstart", "a1", "v1", "amax", "vmax", "dmax", "d1", "vstop"	})},
	{eStepDirect,	'g', "Set SixPoint Simple",	"MVAVAV",	RetArray2({"Stepper", "vstart/vstop", "a1/d1", "v1", "amax/dmax", "vmax"	})},
	{eStepDirect,	'g', "Set Trapezoidal 3",	"MAVA",		RetArray2({"Stepper", "amax", "vmax", "dmax"	})},

	{eStepDirect,	'h', "Set Ramp Mode",		"MR",		RetArray2({"Stepper", "Ramp Mode"					})},

	{eStepDirect,	'i', "Set Timer",			"Mt",		RetArray2({"Stepper", "Time"						})},
	{eStepDirect,	'j', "Wait",				"Mwb",		RetArray2({"Stepper", "Wait for", "Check Timer"		})},

//	{eStepDirect,	'k', "Wait Stop",			"M",		RetArray2({"Stepper"								})},

	{eStepDirect,	'l', "InitGoto",			"MVVAAV",	RetArray2({"Stepper", "StartVelocity", "StopVelocity", "FirstAcceleration", "SecondDeceleration", "FirstVelocity"})},
	{eStepDirect,	'm', "Set Free Running",	"MVml",		RetArray2({"Stepper", "SpeedFor1RPS", "MicroSteps", "Direction"		})},
	{eStepDirect,	'n', "Set Accelerations",	"MaA",		RetArray2({"Stepper", "Acc/De-celeration type", "Accel. Value"		})},
	{eStepDirect,	'o', "Set Velocities",		"MdV",		RetArray2({"Stepper", "Velocity type", "Velocity Value"				})},
	{eStepDirect,	'p', "Set Direction",		"Ml",		RetArray2({"Stepper", "Direction"			})},

	{eStepDirect,	'q', "Set GCONF 1",		"Mbbbbb",		RetArray2({"Stepper", "I_scale_analog", "internal_Rsense", "en_pwm_mode", "enc_commutation", "shaft"})},
	{eStepDirect,	'q', "Set GCONF 2",		"Mbbbbbbb",		RetArray2({"Stepper", "diag0_error", "diag0_otpw", "diag0_stall", "diag1_stall", "diag1_index", "diag1_onstate", "diag1_steps_skipped"	})},
	{eStepDirect,	'q', "Set GCONF 3",		"Mbbbb",			RetArray2({"Stepper", "small_hysteresis", "stop_enable", "direct_mode", "test_mode"																				})},


	{eSteppersFSA,	'f', "FreeRotation",		"Ml",			RetArray2({"Stepper", "l_direction"})},
	{eSteppersFSA,	'w', "WaitPrevCommand",		"M",			RetArray2({"Stepper"})},
	{eSteppersFSA,	'C', "Set Currents",		"Miij",			RetArray2({"Stepper", "IHOLD", "IRUN", "IHOLDDELAY"})},

	{eSteppersFSA,	'h', "Halt",				"MA",			RetArray2({"Stepper", "Deceleration"})},
	{eSteppersFSA,	'G', "Goto",				"MAVS",			RetArray2({"Stepper", "Acceleration", "Velocity", "Steps"})},

	{eSteppersFSA,	'x', "Demo1",				"MbeoMO",		RetArray2({"Stepper", "t_False/True","t_Disable/Enable","t_Open/Close", "t_X/Y/Z/P","t_Off/On"})},
	{eSteppersFSA,	'y', "Demo2",				"MCsAtM",		RetArray2({"Stepper", "t_byte","t_Speed","t_Accel", "t_Time","t_Motor"})},
	{eSteppersFSA,	'y', "Demo3",				"MSVA",			RetArray2({"Stepper", "t_Step","t_Vel","t_Acc.n"})},

	{eSteppersFSA,	'E', "GoEnd",				"M",			RetArray2({"Stepper"})},
	{eSteppersFSA,	'E', "GoEnd 1",				"MV",			RetArray2({"Stepper", "Velocity"})},
	{eSteppersFSA,	'H', "Home",				"M",			RetArray2({"Stepper"})},
	{eSteppersFSA,	'H', "Home 1",				"MV",			RetArray2({"Stepper", "Velocity"})},
	{eSteppersFSA,	'T', "Set Trapezoidal",		"MAc",			RetArray2({"Stepper", "Acceleration", "Velocity"})},
	{eSteppersFSA,	'w', "Wait End Of",			"MMt",			RetArray2({"Stepper", "Motor", "delay"})},

	{eADCConverter,	'a', "GetADC",			"",				RetArray2({							})},

	{eBarCode,		'l', "Laser ON",		"O",			RetArray2({"Laser On/Off"			})},
	{eBarCode,		'R', "Read List",		"",				RetArray2({							})},

	{eStripLed,		'l', "Led Effect",		"g",			RetArray2({"Strip LED Effect"		})},
	{eStripLed,		'd', "Delay",			"t",			RetArray2({"Delay"					})},
	{eStripLed,		'r', "Reset Timer",		"",				RetArray2({							})},
	{eStripLed,		'g', "Get Remaining",	"",				RetArray2({							})},
	{eStripLed,		'w', "WaitTimer",		"",				RetArray2({							})},
	{eStripLed,		'n', "SetNumber",		"n",			RetArray2({"Number To Show"			})},

	{ePwReader,		'v', "Read Volt",		"",				RetArray2({							})},
	{ePwReader,		'A', "Read Amp",		"",				RetArray2({							})},
	{ePwReader,		'W', "Read Watt",		"",				RetArray2({							})},

	{eExpanders,	'v', "Set Output",		"vO",			RetArray2({"Canale Out", "On/Off"	})},
};

void sSampler_Check(void) {
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		const char	ToTest_SubSys	= Sampler_Commands[i].SubSys;
		const char	ToTest_Cmd		= Sampler_Commands[i].cmd;
		unsigned int ToTest_PatLen	= strlen(Sampler_Commands[i].ParamPattern);

		if (ToTest_PatLen > 10 )	//WXSIZEOF(CmdParLabel) defined in CmdEditorCtrl.h
			wxMessageBox(wxString::Format("Too many parameters for ['%c' %u] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);

		if(ToTest_PatLen!= Sampler_Commands[i].ParNames.size())
			wxMessageBox(wxString::Format("Not enaugh names for ['%c' %u] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);

		for (size_t j = i+1; j < WXSIZEOF(Sampler_Commands); j++) {
			if (   ToTest_Cmd == Sampler_Commands[j].cmd
				&& ToTest_PatLen == strlen(Sampler_Commands[j].ParamPattern)
				&& ToTest_SubSys == Sampler_Commands[j].SubSys
			) {
				wxMessageBox(wxString::Format("Duplicated command ['%c' %u] in 'sSampler_Commands'", ToTest_Cmd, ToTest_PatLen), "Error", wxOK | wxICON_INFORMATION, NULL);
			}
		}
	}
	//Check Unused SamplerParams
	for (size_t p = 0; p < WXSIZEOF(SamplerParams); p++) {
		bool Chk = false;
		for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
			if (std::strchr(Sampler_Commands[i].ParamPattern, SamplerParams[p].ParId) != nullptr)
				Chk = true;
		}
		if(Chk==false)
			wxMessageBox(wxString::Format("Parameter '%c' not used!", SamplerParams[p].ParId), "Warning", wxOK | wxICON_INFORMATION, NULL);
	}

	//Check eStepNoMotor/eStepDirect
	for (size_t i = 0; i < WXSIZEOF(Sampler_Commands); i++) {
		if (Sampler_Commands[i].SubSys == eStepNoMotor && Sampler_Commands[i].cmd!='1') {
			//Search if exists the same in eStepDirect
			wxString Pattern = "M"; Pattern.Append(Sampler_Commands[i].ParamPattern);
			const sSampler_Commands*P=Command_GetByCmd(eStepDirect, Sampler_Commands[i].cmd, Pattern.Len());
			if(!P || P->ParamPattern!=Pattern)
				wxMessageBox(wxString::Format("Missing eStepDirect commad '%c'!", Sampler_Commands[i].cmd), "Warning", wxOK | wxICON_INFORMATION, NULL);

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
