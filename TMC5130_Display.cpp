
#include "TMC5130_Display.h"


static char* SwitchMode_v[] = { "==== Switch Mode ===",
                                "stop_l_enable....: ",
                                "stop_r_enable....: ",
                                "pol_stop_l.......: ",
                                "pol_stop_r.......: ",
                                "swap_lr..........: ",
                                "latch_l_active...: ",
                                "latch_l_inactive.: ",
                                "latch_r_active...: ",
                                "latch_r_inactive.: ",
                                "en_latch_encoder.: ",
                                "sg_stop..........: ",
                                "en_softstop......: "
                              };
static char* SwitchMode_h[] = {"SW_MODE...: |","stopLen|","stopRen|","polStopL|","polStopR|","swapLR|","latLact|","latLinac|","latRact|","latRinac|","enLatEnc|","sgStop|","softStop|"};
static char* SwitchMode_x[] = {"            |","       |","       |","        |","        |","      |","       |","        |","       |","        |","        |","      |","        |"};


static char* RampStatus_v[] = { "===== RAMP_STAT =====",
                                "status_stop_l.....: ",
                                "status_stop_r.....: ",
                                "status_latch_l....: ",
                                "status_latch_r....: ",
                                "event_stop_l......: ",
                                "event_stop_r......: ",
                                "event_stop_sg.....: ",
                                "event_pos_reached.: ",
                                "velocity_reached..: ",
                                "position_reached..: ",
                                "vzero.............: ",
                                "t_zerowait_active.: ",
                                "second_move.......: ",
                                "status_sg.........: ",
};
static char* RampStatus_h[] = {"RAMP_STAT.: |","stStopL|","stStopR|","stLatL|","stLatR|","evStpL|","evStpR|","evStpSg|","evPosRch|","velRch|","posRch|","vZero|","t_0wait|","secMov|","stSg|"};
static char* RampStatus_x[] = {"            |","       |","       |","      |","      |","      |","      |","       |","        |","      |","      |","     |","       |","      |","    |"};


static char* SpiStatus_v[] = {  "==== SPI STATUS ===",
                                "reset_flag......: ",
                                "driver_error....: ",
                                "StallGuard2.....: ",
                                "standstill......: ",
                                "velocity_reached: ",
                                "position_reached: ",
                                "status_stop_l...: ",
                                "status_stop_r...: ",
                              };
static char* SpiStatus_h[] = {"SPI STATUS: |","reset|","drvErr|","sg2|","stdStil|","velOK|","posOK|","     |","stopR|"};
static char* SpiStatus_x[] = {"            |","     |","      |","   |","       |","     |","     |","stopL|","     |"};


static char* Inputs_v[] = { "===== INPUTS =====",
                            "REFL_STEP......: ",
                            "REFR_DIR.......: ",
                            "ENCB_DCEN_CFG4.: ",
                            "ENCB_DCEN_CFG5.: ",
                            "DRV_ENN_CFG6...: ",
                            "ENC_N_DCO......: ",
                            "SD_MODE........: ",
                            "SWCOMP_IN......: ",
};
static char* Inputs_h[] = {"INPUTS....: |","REFL|","REFR|","CFG4|","CFG5|","CFG6|","DCO|","SDMD|","SWCP|"};
static char* Inputs_x[] = {"            |","    |","    |","    |","    |","    |","   |","    |","    |"};

static char* GStatus_v[] = {  "===== GSTATUS =====",
                              "reset_flag......: ",
                              "driver_error....: ",
                              "undervoltage....: ",
              };
static char* GStatus_h[] = {"GSTATUS...: |","reset|","drvErr|","underVolt|"};
static char* GStatus_x[] = {"            |","     |","      |","         |"};

static char* Actuals_v[] = {"===== ACTUALS =====",
                            "XACTUAL...: ",
                            "XTARGET...: ",
                            "X_COMPARE.: ",
                            "TSTEP.....: ",
                            "VACTUAL...: ",
                            "RAMP_STAT.: ",
                            "XLATCH....: ",
                            "MSCURACT..: ",
                            "Cur_A.....: ",
                            "Cur_B.....: ",
              };
static char* Actuals_h[] = {"ACTUALS...: |","XACTUAL|","XTARGET|","X_COMPARE|","TSTEP|","VACTUAL|","RAMP_STAT|","XLATCH|","MSCURACT|","Cur_A|","Cur_B|"};
static char* Actuals_x[] = {"            |","       |","       |","         |","     |","       |","         |","      |","        |","     |","     |"};


void GenShow(uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle, uint32_t s, char* Vnames[], char* Hnames[], char* Xnames[], uint8_t Size){
  uint32_t Mask = 0x01;
  if(Vert){
    if(ShowTitle) ansi.gotoXY(Col, Row++);  ansi.print(Vnames[0]);
    for(uint8_t i=1; i<Size; i++){
      ansi.gotoXY(Col, Row++);  ansi.print(Vnames[i]);  ansi.print(s&Mask?1:0); Mask<<=1;
    }
  } else {
    ansi.gotoXY(Col, Row);
    if(ShowTitle) ansi.print(Hnames[0]);
    for(uint8_t i=1; i<Size; i++){
      ansi.print(s&Mask ? Hnames[i]: Xnames[i]);    Mask<<=1;     
    }
  }
}

void GenShowReg(ShowAsTyp Typ, uint32_t Val, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  char** Vnames;
  char** Hnames;
  char** Xnames;
  uint8_t Size = 0;
  switch(Typ){
    case ShowAsSwitch:    Vnames = SwitchMode_v;  Hnames = SwitchMode_h;  Xnames = SwitchMode_x;  Size = sizeof(SwitchMode_v)/sizeof(SwitchMode_v[0]);  break;
    case ShowAsRamp:      Vnames = RampStatus_v;  Hnames = RampStatus_h;  Xnames = RampStatus_x;  Size = sizeof(RampStatus_v)/sizeof(RampStatus_v[0]);  break;
    case ShowAsSpiStatus: Vnames =  SpiStatus_v;  Hnames =  SpiStatus_h;  Xnames =  SpiStatus_x;  Size = sizeof( SpiStatus_v)/sizeof( SpiStatus_v[0]);  break;
    case ShowAsInputs:    Vnames =     Inputs_v;  Hnames =     Inputs_h;  Xnames =     Inputs_x;  Size = sizeof(    Inputs_v)/sizeof(    Inputs_v[0]);  break;
    case ShowAsGStatus:   Vnames =    GStatus_v;  Hnames =    GStatus_h;  Xnames =    GStatus_x;  Size = sizeof(   GStatus_v)/sizeof(   GStatus_v[0]);  break;
    default: return;
  }
  GenShow(Col, Row, Vert, ShowTitle, Val, Vnames, Hnames, Xnames, Size);
}

uint16_t ShowSwitchMode(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  uint16_t Val = stp->readReg(TMC5130::SW_MODE);
  GenShow(Col, Row, Vert, ShowTitle, Val, SwitchMode_v, SwitchMode_h, SwitchMode_x, sizeof(SwitchMode_v)/sizeof(SwitchMode_v[0]));
  return Val;
}

void ShowRampStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  GenShow(Col, Row, Vert, ShowTitle, stp->readReg(TMC5130::RAMP_STAT), RampStatus_v, RampStatus_h, RampStatus_x, sizeof(RampStatus_v)/sizeof(RampStatus_v[0]));
}

uint8_t ShowSpiStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  uint8_t Val = stp->GetSpiStatus();
  GenShow(Col, Row, Vert, ShowTitle, Val, SpiStatus_v, SpiStatus_h, SpiStatus_x, sizeof(SpiStatus_v)/sizeof(SpiStatus_v[0]));
  return Val;
}

uint8_t ShowInputs(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  uint8_t Val = stp->readReg(TMC5130::IOIN);
  GenShow(Col, Row, Vert, ShowTitle, Val, Inputs_v, Inputs_h, Inputs_x, sizeof(Inputs_v)/sizeof(Inputs_v[0]));
  return Val;
}

void ShowGStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  GenShow(Col, Row, Vert, ShowTitle, stp->readReg(TMC5130::GSTAT), GStatus_v, GStatus_h, GStatus_x, sizeof(GStatus_v)/sizeof(GStatus_v[0]));
}

void ShowActuals(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
    if(ShowTitle){
      ansi.gotoXY(Col, Row++);  ansi.print("===== ACTUALS =====");
    }
    ansi.gotoXY(Col, Row++);  ansi.print("XACTUAL...: ");  ansi.print( (int32_t)stp->readReg(TMC5130::XACTUAL  ));        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("XTARGET...: ");  ansi.print( (int32_t)stp->readReg(TMC5130::XTARGET  ));        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("X_COMPARE.: ");  ansi.print(          stp->readReg(TMC5130::X_COMPARE));        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("TSTEP.....: ");  ansi.print(          stp->readReg(TMC5130::TSTEP)&0xFFFFF);        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("VACTUAL...: ");  ansi.print(          stp->getVelocity()              );        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("RAMP_STAT.: ");  ansi.print(          stp->readReg(TMC5130::RAMP_STAT), HEX);   ansi.print("       ");    
    ansi.gotoXY(Col, Row++);  ansi.print("XLATCH....: ");  ansi.print(          stp->readReg(TMC5130::XLATCH   )     );   ansi.print("       ");

    uint32_t MsCurAct =  stp->readReg(TMC5130::MSCURACT);
    ansi.gotoXY(Col, Row++);  ansi.print("MSCURACT..: ");  ansi.print(MsCurAct,HEX);                          ansi.print("          ");
    ansi.gotoXY(Col, Row++);  ansi.print("Cur_A.....: ");  ansi.print((uint16_t)( MsCurAct      & 0x1FF));    ansi.print("   ");
    ansi.gotoXY(Col, Row++);  ansi.print("Cur_B.....: ");  ansi.print((uint16_t)((MsCurAct>>16) & 0x1FF));    ansi.print("   ");
}

void ShowActualsFast(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle) {
    if(ShowTitle){
      ansi.gotoXY(Col, Row++);  ansi.print("===== ACTUALS =====");
    }
    uint32_t r = 0;
        stp->genSpiFunct(TMC5130::XACTUAL, 0, true);  //1) Dummy reading, but prepare next result
    r = stp->genSpiFunct(TMC5130::XTARGET,   0, true);  ansi.gotoXY(Col, Row++);  ansi.print("XACTUAL...: ");  ansi.print((int32_t)r);        ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::X_COMPARE, 0, true);  ansi.gotoXY(Col, Row++);  ansi.print("XTARGET...: ");  ansi.print((int32_t)r);        ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::TSTEP,     0, true);  ansi.gotoXY(Col, Row++);  ansi.print("X_COMPARE.: ");  ansi.print(r         );        ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::VACTUAL,   0, true);  ansi.gotoXY(Col, Row++);  ansi.print("TSTEP.....: ");  ansi.print(r&0xFFFFF );        ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::RAMP_STAT, 0, true);  ansi.gotoXY(Col, Row++);  ansi.print("VACTUAL...: ");  ansi.print(((int32_t)(r<<8))>>8);        ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::XLATCH,    0, true);  ansi.gotoXY(Col, Row++);  ansi.print("RAMP_STAT.: ");  ansi.print(r, HEX);   ansi.print("       ");    
    r = stp->genSpiFunct(TMC5130::MSCURACT,  0, true);  ansi.gotoXY(Col, Row++);  ansi.print("XLATCH....: ");  ansi.print(r     );   ansi.print("       ");
    r = stp->genSpiFunct(TMC5130::XACTUAL,   0, true);  ansi.gotoXY(Col, Row++);  ansi.print("MSCURACT..: ");  ansi.print(r, HEX);   ansi.print("          ");
                                                        ansi.gotoXY(Col, Row++);  ansi.print("Cur_A.....: ");  ansi.print((uint16_t)( r      & 0x1FF));    ansi.print("   ");
                                                        ansi.gotoXY(Col, Row++);  ansi.print("Cur_B.....: ");  ansi.print((uint16_t)((r>>16) & 0x1FF));    ansi.print("   ");
}


void ShowCoolConf(uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true){
    uint32_t cf = 0x1234567890;//stp->readReg(TMC5130::COOLCONF);
    if(ShowTitle){
      ansi.gotoXY(Col, Row++);  ansi.print("===== COOLCONF =====");
    }
    ansi.gotoXY(Col, Row++);  ansi.print("COOLCONF...: ");  ansi.print( cf, HEX );        ansi.print("       ");
    ansi.gotoXY(Col, Row++);  ansi.print("SeMin...: ");  ansi.print( cf & 0x0F, HEX );        ansi.print("       ");
}

void ShowWaiting(TMC5130 *stp, const float waiting, bool WaitStop){
  unsigned long time = millis();
  ansi.clearScreen();
  while (millis() - time < waiting ) {
    ansi.gotoXY(1, 1);
    ansi.print("MicroSteps: "); ansi.print(stp->getMicrosteps());
    ansi.print(" CHOPCONF: ");  ansi.print(stp->readReg(TMC5130::CHOPCONF), HEX);
    ansi.print(" Ic Vers.: ");  ansi.print(stp->getIcVersion(), HEX);

//    ansi.print(" Pos: ");       ansi.print(stp->getPosition());
//    ansi.print(" Speed: ");     ansi.print(stp->getVelocity());
    ansi.print(" time: ");      ansi.print(millis()); ansi.print("              ");

    ShowSpiStatus (stp,  1, 10);   ShowSpiStatus (stp, 1,3,false);
    ShowInputs    (stp, 25, 10);   ShowInputs    (stp, 1,4,false);
    ShowSwitchMode(stp,  1, 20);   ShowSwitchMode(stp, 1,5, false);
    ShowActuals   (stp, 25, 20);
    ShowCoolConf  (50, 10);

    uint8_t St = stp->GetSpiStatus();
    ansi.gotoXY(1, 8); ansi.print("Spi Status: ");ansi.print(St, HEX);
    ansi.gotoXY(1, 9); ansi.print("Is Stopped: ");ansi.print( (St&0x20)?"Yes":"No ");
/*
    //ShowGStatus   (stp, 50,  2);  ShowGStatus   (stp, 1,14,false);  //Note: GSTAT is R+C (Cleared after Readed) 
    //ShowRampStatus(stp, 35, 18);  ShowRampStatus(stp, 1,15,false);  //Note:RAMP_STAT is R+C (Cleared after Readed) 
*/    
    uint8_t s = stp->GetSpiStatus() & 0x20;
    if(WaitStop && (stp->GetSpiStatus() & 0x20)) return;
  }
}
