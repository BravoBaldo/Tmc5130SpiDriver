
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
    case ShowAsSwitch:    Vnames = SwitchMode_v;  Hnames = SwitchMode_h;  Xnames = SwitchMode_x;  Size = wxSIZEOF(SwitchMode_v);  break;
    case ShowAsRamp:      Vnames = RampStatus_v;  Hnames = RampStatus_h;  Xnames = RampStatus_x;  Size = wxSIZEOF(RampStatus_v);  break;
    case ShowAsSpiStatus: Vnames =  SpiStatus_v;  Hnames =  SpiStatus_h;  Xnames =  SpiStatus_x;  Size = wxSIZEOF( SpiStatus_v);  break;
    case ShowAsInputs:    Vnames =     Inputs_v;  Hnames =     Inputs_h;  Xnames =     Inputs_x;  Size = wxSIZEOF(    Inputs_v);  break;
    case ShowAsGStatus:   Vnames =    GStatus_v;  Hnames =    GStatus_h;  Xnames =    GStatus_x;  Size = wxSIZEOF(   GStatus_v);  break;
    default: return;
  }
  GenShow(Col, Row, Vert, ShowTitle, Val, Vnames, Hnames, Xnames, Size);
}

uint16_t ShowSwitchMode(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){  //SW_MODE
  uint16_t Val = stp->getSwMode().bytes;
  GenShow(Col, Row, Vert, ShowTitle, Val, SwitchMode_v, SwitchMode_h, SwitchMode_x, wxSIZEOF(SwitchMode_v));
  return Val;
}

void ShowRampStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){ //RAMP_STAT
  GenShow(Col, Row, Vert, ShowTitle, stp->getRampStat().bytes, RampStatus_v, RampStatus_h, RampStatus_x, wxSIZEOF(RampStatus_v));
}

uint8_t ShowSpiStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
  uint8_t Val = stp->GetSpiStatus().bytes;
  GenShow(Col, Row, Vert, ShowTitle, Val, SpiStatus_v, SpiStatus_h, SpiStatus_x, wxSIZEOF(SpiStatus_v));
  return Val;
}

uint8_t ShowInputs(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){  //IOIN
  uint8_t Val = stp->getIoin().bytes;
  GenShow(Col, Row, Vert, ShowTitle, Val, Inputs_v, Inputs_h, Inputs_x, wxSIZEOF(Inputs_v));
  return Val;
}

void ShowGStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){  //GSTAT
  GenShow(Col, Row, Vert, ShowTitle, stp->getGstat().bytes, GStatus_v, GStatus_h, GStatus_x, wxSIZEOF(GStatus_v));
}

void ShowCurrents(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){ //MSCURACT
  uint32_t act = stp->getMscuract().bytes;

  if(ShowTitle){
    ansi.gotoXY(Col, Row++);  ansi.print("==== CURRENTS ====");
  }
  ansi.gotoXY(Col, Row++);  ansi.print("MSCURACT: ");  ansi.print(          act, HEX          );        ansi.print("       ");
  ansi.gotoXY(Col, Row++);  ansi.print("CUR_B...: ");  ansi.print( (int8_t)( act      & 0x1FF));        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("CUR_A...: ");  ansi.print( (int8_t)((act>>16) & 0x1FF));        ansi.print("    ");
}

void ShowDrvStatus(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){  //DRV_STATUS
  uint32_t drv = stp->getDrvStatus().bytes;
  if(ShowTitle){
    ansi.gotoXY(Col, Row++);  ansi.print("==== DRV_STATUS ====");
  }
  ansi.gotoXY(Col, Row++);  ansi.print("DRV_STATUS..: ");  ansi.print(          drv, HEX       );        ansi.print("       ");
  ansi.gotoXY(Col, Row++);  ansi.print("SG_RESULT...: ");  ansi.print( (uint8_t)((drv) & 0x3FF));        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("fsactive....: ");  ansi.print( (drv&0x00008000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("CS ACTUAL...: ");  ansi.print( (uint8_t)((drv>>16) & 0x1F));        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("StallGuard..: ");  ansi.print( (drv&0x01000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("ot..........: ");  ansi.print( (drv&0x02000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("otpw........: ");  ansi.print( (drv&0x04000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("s2ga........: ");  ansi.print( (drv&0x08000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("s2gb........: ");  ansi.print( (drv&0x10000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("ola.........: ");  ansi.print( (drv&0x20000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("olb.........: ");  ansi.print( (drv&0x40000000)?1:0 );        ansi.print("    ");
  ansi.gotoXY(Col, Row++);  ansi.print("stst........: ");  ansi.print( (drv&0x80000000)?1:0 );        ansi.print("    ");

}

void ShowActuals(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle){
    if(ShowTitle){
      ansi.gotoXY(Col, Row++);  ansi.print("===== ACTUALS =====");
    }
    ansi.gotoXY(Col, Row++);  ansi.print("XACTUAL...: ");  ansi.print( (int32_t)stp->getPosition()            );  ansi.print("       "); //XACTUAL
    ansi.gotoXY(Col, Row++);  ansi.print("XTARGET...: ");  ansi.print( (int32_t)stp->getTarget()              );  ansi.print("       "); //XTARGET
    ansi.gotoXY(Col, Row++);  ansi.print("X_COMPARE.: ");  ansi.print(          stp->getXCompare()            );  ansi.print("       "); //AAA X_COMPARE WriteOnly
    ansi.gotoXY(Col, Row++);  ansi.print("TSTEP.....: ");  ansi.print(          stp->getTStep()               );  ansi.print("       "); //TSTEP
    ansi.gotoXY(Col, Row++);  ansi.print("VACTUAL...: ");  ansi.print(          stp->getVelocity()            );  ansi.print("       "); //VACTUAL
    ansi.gotoXY(Col, Row++);  ansi.print("RAMP_STAT.: ");  ansi.print(          stp->getRampStat().bytes, HEX );  ansi.print("       "); //RAMP_STAT
    ansi.gotoXY(Col, Row++);  ansi.print("XLATCH....: ");  ansi.print(          stp->getXLatch()              );  ansi.print("       "); //XLATCH

    TMC5130::Mscuract CurAct =  stp->getMscuract();  //MSCURACT

    ansi.gotoXY(Col, Row++);  ansi.print("MSCURACT..: ");  ansi.print(CurAct.bytes,HEX);                          ansi.print("          ");
    ansi.gotoXY(Col, Row++);  ansi.print("Cur_A.....: ");  ansi.print(CurAct.cur_a);    ansi.print("   ");
    ansi.gotoXY(Col, Row++);  ansi.print("Cur_B.....: ");  ansi.print(CurAct.cur_b);    ansi.print("   ");
}

/*
void ShowActualsFast(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert, bool ShowTitle) {
    if(ShowTitle){
      ansi.gotoXY(Col, Row++);  ansi.print("===== ACTUALS =====");
    }
    uint32_t r = 0;
        stp->genSpiFunct(TMC5130::XACTUAL,   0, true);  //1) Dummy reading, but prepare next result
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
*/
void ShowCoolConf(uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true){
  //getCoolconf
    uint32_t cf = 0x1234567890;//stp->getCoolconf().bytes; //readReg(TMC5130::COOLCONF); //COOLCONF AAA WriteOnly
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
    ansi.print(" CHOPCONF: ");  ansi.print(stp->getChopconf().bytes, HEX);  //CHOPCONF
    ansi.print(" Ic Vers.: ");  ansi.print(stp->getIcVersion(), HEX);

//    ansi.print(" Pos: ");       ansi.print(stp->getPosition());
//    ansi.print(" Speed: ");     ansi.print(stp->getVelocity());
    ansi.print(" time: ");      ansi.print(millis()); ansi.print("              ");

    ShowSpiStatus (stp,  1, 10);   ShowSpiStatus (stp, 1,3,false);
    ShowInputs    (stp, 25, 10);   ShowInputs    (stp, 1,4,false);
    ShowSwitchMode(stp,  1, 20);   ShowSwitchMode(stp, 1,5, false);
    ShowActuals   (stp, 25, 20);
    ShowCoolConf  (50, 10);

    TMC5130::SpiStatus St = stp->GetSpiStatus();
    ansi.gotoXY(1, 8); ansi.print("Spi Status: ");ansi.print(St.bytes, HEX);
    ansi.gotoXY(1, 9); ansi.print("Is Stopped: ");ansi.print( (St.position_reached)?"Yes":"No ");
/*
    //ShowGStatus   (stp, 50,  2);  ShowGStatus   (stp, 1,14,false);  //Note: GSTAT is R+C (Cleared after Readed) 
    //ShowRampStatus(stp, 35, 18);  ShowRampStatus(stp, 1,15,false);  //Note:RAMP_STAT is R+C (Cleared after Readed) 
*/    
    if(WaitStop && (stp->GetSpiStatus().position_reached)) return;
  }
}


void Printer::printRegisterPortion(const char * str, uint32_t value, int base) {
  print_ptr_->print(str);
  switch (base) {
    case BIN: print_ptr_->print(": 0b");  break;
    case HEX: print_ptr_->print(": 0x");  break;
    default:  print_ptr_->print(": ");    
  }
  print_ptr_->print(value, base);
  print_ptr_->println();
}

void Printer::readAndPrintGconf(TMC5130 &stepper){//GCONF
  printRegister(stepper.getGconf());
}

void Printer::printRegister(TMC5130::Gconf gconf){
  printRegisterPortion("gconf",                           gconf.bytes,                          HEX);
  printRegisterPortion("recalibrate_i_scale_analog",      gconf.recalibrate_i_scale_analog,     BIN);
  printRegisterPortion("faststandstill_internal_rsense",  gconf.faststandstill_internal_rsense, BIN);
  printRegisterPortion("en_pwm_mode",                     gconf.en_pwm_mode,                    BIN);
  printRegisterPortion("multistep_filt_enc_commutation",  gconf.multistep_filt_enc_commutation, BIN);
  printRegisterPortion("shaft",                           gconf.shaft,                          BIN);
  //printRegisterPortion("diag0_error",                     gconf.diag0_error,                    BIN);
  //printRegisterPortion("diag0_otpw",                      gconf.diag0_otpw,                     BIN);
  printRegisterPortion("diag0_stall_int_step",            gconf.diag0_stall_int_step,           BIN);
  printRegisterPortion("diag1_stall_poscomp_dir",         gconf.diag1_stall_poscomp_dir,        BIN);
  //printRegisterPortion("diag1_index",                     gconf.diag1_index,                    BIN);
  //printRegisterPortion("diag1_onstate",                   gconf.diag1_onstate,                  BIN);
  //printRegisterPortion("diag1_steps_skipped",             gconf.diag1_steps_skipped,            BIN);
  printRegisterPortion("diag0_int_pushpull",              gconf.diag0_int_pushpull,             BIN);
  printRegisterPortion("diag1_poscomp_pushpull",          gconf.diag1_poscomp_pushpull,         BIN);
  printRegisterPortion("small_hysteresis",                gconf.small_hysteresis,               BIN);
  printRegisterPortion("stop_enable",                     gconf.stop_enable,                    BIN);
  printRegisterPortion("direct_mode",                     gconf.direct_mode,                    BIN);
  printRegisterPortion("test_mode",                       gconf.test_mode,                      BIN);
  
  print_ptr_->println("--------------------------");
}




void Printer::printRegister(TMC5130::PwmScale & pwm_scale){
  printRegisterPortion("pwm_scale",     pwm_scale.bytes,          HEX);
  printRegisterPortion("pwm_scale_sum", pwm_scale.pwm_scale_sum,  DEC);
  print_ptr_->println("--------------------------");
}

void Printer::readAndPrintPwmScale(TMC5130 &stepper) {  //PWM_SCALE
  TMC5130::PwmScale pwm_scale = stepper.getPwmScale();
  printRegister(pwm_scale);
}

void ShowReadableRegisters(TMC5130 &stepper){
//   print_ptr_->println("--------------------------");
 Serial.println("---Readable Registers---");
  for(int i=0; i<(wxSIZEOF(TMC5130::RegsReadable)); i++){
    Serial.printf("%02d) %02X: %08X\n", i, TMC5130::RegsReadable[i], stepper.readReg(TMC5130::RegsReadable[i]));
/*    Serial.print(i);
    Serial.print(") ");
    Serial.print(TMC5130::RegsReadable[i], HEX);
    Serial.print(":");
    Serial.println(stepper.readReg(TMC5130::RegsReadable[i]), HEX);
*/
  }
  Serial.println("-------------------------------\n");
}