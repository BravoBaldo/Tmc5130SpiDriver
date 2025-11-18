
#include "TMC5130_Inits.h"

void TMC5130_Init_00(TMC5130 &stepper){
  stepper.writeReg(TMC5130::GCONF,       0);
  stepper.writeReg(TMC5130::GSTAT,       0);
  stepper.writeReg(TMC5130::IFCNT,       0);
  stepper.writeReg(TMC5130::SLAVECONF,   0);
  stepper.writeReg(TMC5130::IOIN,        0x1100002F); //TMC5130_IOIN_OUTPUT
  stepper.writeReg(TMC5130::X_COMPARE,   0x00000000);
  stepper.writeReg(TMC5130::IHOLD_IRUN,  0x00070101);  //
  
  stepper.writeReg(TMC5130::TPOWERDOWN,  0x00000000);
  stepper.writeReg(TMC5130::TSTEP,       0x0000004D);
  stepper.writeReg(TMC5130::TPWMTHRS,    0x00000000);
  stepper.writeReg(TMC5130::TCOOLTHRS,   0x00000000);
  stepper.writeReg(TMC5130::THIGH,       0x00000000);
  stepper.writeReg(TMC5130::RAMPMODE,    0x00000002);
  stepper.writeReg(TMC5130::XACTUAL,     0xFFE38782);
  stepper.writeReg(TMC5130::VACTUAL,     0x00FCB924);
  stepper.writeReg(TMC5130::VSTART,      0x00000000);
  stepper.writeReg(TMC5130::A1,          0x000003C6);
  stepper.writeReg(TMC5130::V1,          0x0001A36E);

  stepper.writeReg(TMC5130::AMAX,        0x000003C6);
  stepper.writeReg(TMC5130::VMAX,        0x000346DC);
  stepper.writeReg(TMC5130::DMAX,        0x000003C6);
  stepper.writeReg(TMC5130::D1,          0x000003C6);
  stepper.writeReg(TMC5130::VSTOP,       0x0000000A);
  stepper.writeReg(TMC5130::TZEROWAIT,   0x00000000);
  stepper.writeReg(TMC5130::XTARGET,     0x00002710);
  stepper.writeReg(TMC5130::VDCMIN,      0x00000000);
  stepper.writeReg(TMC5130::SW_MODE,     0x00000000);
  stepper.writeReg(TMC5130::RAMP_STAT,   0x00000103);
  stepper.writeReg(TMC5130::XLATCH,      0x00000000);
  stepper.writeReg(TMC5130::ENCMODE,     0x00000000);
  stepper.writeReg(TMC5130::X_ENC,       0x00000000);
  stepper.writeReg(TMC5130::ENC_CONST,   0x00010000);
  stepper.writeReg(TMC5130::ENC_STATUS,  0x00000000);
  stepper.writeReg(TMC5130::ENC_LATCH,   0x00000000);

  //reset default microstep table:
  stepper.writeReg(TMC5130::MSLUT_0,     0xAAAAB554);  //0xAAAAB554
  stepper.writeReg(TMC5130::MSLUT_1,     0x4A9554AA);  //0x4A9554AA
  stepper.writeReg(TMC5130::MSLUT_2,     0x24492929);  //0x24492929
  stepper.writeReg(TMC5130::MSLUT_3,     0x10104222);  //0x10104222
  stepper.writeReg(TMC5130::MSLUT_4,     0xFBFFFFFF);  //0xFBFFFFFF
  stepper.writeReg(TMC5130::MSLUT_5,     0xB5BB777D);  //0xB5BB777D
  stepper.writeReg(TMC5130::MSLUT_6,     0x49295556);  //0x49295556
  stepper.writeReg(TMC5130::MSLUT_7,     0x00404222);  //0x00404222
  stepper.writeReg(TMC5130::MSLUTSEL,    0xFFFF8056);  //0xFFFF8056  X1=128, X2=255, X3=255  W3=%01, W2=%01, W1=%01, W0=%10
  stepper.writeReg(TMC5130::MSLUTSTART,  0x00F70000);  //0x00F70000  START_SIN_0= 0, START_SIN90= 247

  stepper.writeReg(TMC5130::MSCNT,       0x0000006B);
  stepper.writeReg(TMC5130::MSCURACT,    0x00F601E5);
  stepper.writeReg(TMC5130::CHOPCONF,    0x000101D5);  //**** 00010000000111010101
  stepper.writeReg(TMC5130::COOLCONF,    0x00000000);
  stepper.writeReg(TMC5130::DCCTRL,      0x00000000);
  stepper.writeReg(TMC5130::DRV_STATUS,  0x61010000);
  stepper.writeReg(TMC5130::PWMCONF,     0x000500C8);
  stepper.writeReg(TMC5130::PWMSTATUS,   0x00000000); //TMC5130_PWM_SCALE
  stepper.writeReg(TMC5130::ENCM_CTRL,   0x00000000);
  stepper.writeReg(TMC5130::LOST_STEPS,  0x00000000); 
}

void TMC5130_Init_01(TMC5130 &stepper){
                                  //00010000000011000011
  stepper.setChopconf          (65731);  //writeReg(CHOPCONF,    0x000100C3);  //SPI send: 0xEC000100C3; // CHOPCONF: TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle)

//  irun      = 0x1F; //31  Corrente a motore in moto 0...31
//  ihold     = 0xA;  //10  Corrente a motore fermo 0...31
//  holdDelay = 0x06; //6   0: spegnimento istantaneo, 1..15: Ritardo per ogni step di riduzione della corrente in multipli di 2^18 clock
  stepper.setCurrent(31, 10, 6);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6

  stepper.writeReg(TMC5130::TPOWERDOWN,  0x0000000A);  //SPI send: 0x910000000A; // TPOWERDOWN=10: Delay before power down in stand still
  stepper.writeReg(TMC5130::GCONF,       0x00000004);  //SPI send: 0x8000000004; // EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
  stepper.writeReg(TMC5130::TPWMTHRS,    0x000001F4);  //SPI send: 0x93000001F4; // TPWM_THRS=500 yields a switching velocity about 35000 = ca. 30RPM
  stepper.writeReg(TMC5130::PWMCONF,     0x000401C8);  //SPI send: 0xF0000401C8;

  stepper.setFirstAcceleration  (1000);                   //writeReg(A1,          0x000003E8);  //SPI send: 0xA4000003E8; // A1 = 1 000 First acceleration
  stepper.setFirstVelocity      (50000);                  //writeReg(V1,          0x0000C350);  //SPI send: 0xA50000C350; // V1 = 50 000 Acceleration threshold velocity V1
  stepper.setSecondAcceleration (500);                    //writeReg(AMAX,        0x000001F4);  //SPI send: 0xA6000001F4; // AMAX = 500 Acceleration above V1
  stepper.setMaxVelocity        (200000);                 //writeReg(VMAX,        0x00030D40);  //SPI send: 0xA700030D40; // VMAX = 200 000
  stepper.setFirstDeceleration  (700);                    //writeReg(DMAX,        0x000002BC);  //SPI send: 0xA8000002BC; // DMAX = 700 Deceleration above V1
  stepper.setSecondDeceleration (1400);                   //writeReg(D1,          0x00000578);  //SPI send: 0xAA00000578; // D1 = 1400 Deceleration below V1
  stepper.setStopVelocity       (10);                     //writeReg(VSTOP,       0x0000000A);  //SPI send: 0xAB0000000A; // VSTOP = 10 Stop velocity (Near to zero)
  stepper.setRampMode           (TMC5130::TMC5130_MODE_POSITION);  //writeReg(RAMPMODE,    0x00000000);  //SPI send: 0xA000000000; // RAMPMODE = 0 (Target position move)

#define ENABLE_STALLGUARD
//  stepper.StopEnable(true);
  stepper.SetSwMode(0
      | SW_MODE_STOP_L_ENABLE | SW_MODE_POL_STOP_L  //Enable Left Limit Switch
  //  | SW_MODE_STOP_R_ENABLE
  //  | SW_MODE_POL_STOP_R
  //  | SW_MODE_SWAP_LR
#if defined(ENABLE_STALLGUARD)
      | SW_MODE_SG_STOP     //Enable StallGuard2
#endif
  );
  stepper.setCurrent(1, 1, 1);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
  stepper.setMicrosteps(8);
#if defined(ENABLE_STALLGUARD)
  stepper.setTCOOLTHRS(20);
#endif
}

void TMC5130_Init_02(TMC5130 &stepper, bool Start){
  stepper.writeReg((TMC5130::Reg)0x00, 0x00000000);	// writing GCONF @ address 0=0x00 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x03, 0x00000000);	// writing SLAVECONF @ address 1=0x03 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x05, 0x00000000);	// writing X_COMPARE @ address 2=0x05 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x10, 0x00071703);	// writing IHOLD_IRUN @ address 3=0x10 with 0x00071703=464643=0.0
  stepper.writeReg((TMC5130::Reg)0x11, 0x00000000);	// writing TPOWERDOWN @ address 4=0x11 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x13, 0x00000000);	// writing TPWMTHRS @ address 5=0x13 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x14, 0x00000000);	// writing TCOOLTHRS @ address 6=0x14 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x15, 0x00000000);	// writing THIGH @ address 7=0x15 with 0x00000000=0=0.0

  if(Start){
    stepper.setRampMode(TMC5130::TMC5130_MODE_VELPOS); //    stepper.writeReg((TMC5130::Reg)0x20, 0x00000001);	// writing RAMPMODE @ address 8=0x20 with 0x00000001=1=0.0
    stepper.writeReg((TMC5130::Reg)0x21, 0x000901BF);	// writing XACTUAL  @ address 9=0x21 with 0x000901BF=590271=0.0
    stepper.setMaxVelocity(53687);            //stepper.writeReg((TMC5130::Reg)0x27, 0x0000D1B7);	// writing VMAX     @ address 14=0x27 with 0x0000D1B7=53687=0.0
  }else{
    stepper.writeReg((TMC5130::Reg)0x20, 0x00000000);	// writing RAMPMODE @ address 8=0x20 with 0x00000000=0=0.0
    stepper.writeReg((TMC5130::Reg)0x21, 0x00000000);	// writing XACTUAL  @ address 9=0x21 with 0x00000000=0=0.0
    stepper.writeReg((TMC5130::Reg)0x27, 0x00000000);	// writing VMAX     @ address 14=0x27 with 0x00000000=0=0.0
  }
  stepper.writeReg((TMC5130::Reg)0x23, 0x00000000);	// writing VSTART @ address 10=0x23 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x24, 0x00000000);	// writing A1 @ address 11=0x24 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x25, 0x00000000);	// writing V1 @ address 12=0x25 with 0x00000000=0=0.0
  stepper.setSecondAcceleration(100); // stepper.writeReg((TMC5130::Reg)0x26, 0x00000064);	// writing AMAX @ address 13=0x26 with 0x00000064=100=0.0
  stepper.setFirstDeceleration(100);  //stepper.writeReg((TMC5130::Reg)0x28, 0x00000064);	// writing DMAX @ address 15=0x28 with 0x00000064=100=0.0
  stepper.writeReg((TMC5130::Reg)0x2A, 0x00000000);	// writing D1 @ address 16=0x2A with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x2B, 0x00000000);	// writing VSTOP @ address 17=0x2B with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x2C, 0x00000000);	// writing TZEROWAIT @ address 18=0x2C with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x2D, 0x00000000);	// writing XTARGET @ address 19=0x2D with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x33, 0x00000000);	// writing VDCMIN @ address 20=0x33 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x34, 0x00000000);	// writing SW_MODE @ address 21=0x34 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x38, 0x00000000);	// writing ENCMODE @ address 22=0x38 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x39, 0x00000000);	// writing X_ENC @ address 23=0x39 with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x3A, 0x00010000);	// writing ENC_CONST @ address 24=0x3A with 0x00010000=65536=0.0
  stepper.writeReg((TMC5130::Reg)0x60, 0xAAAAB554);	// writing MSLUT_0 @ address 25=0x60 with 0xAAAAB554=0=0.0
  stepper.writeReg((TMC5130::Reg)0x61, 0x4A9554AA);	// writing MSLUT_1 @ address 26=0x61 with 0x4A9554AA=1251300522=0.0
  stepper.writeReg((TMC5130::Reg)0x62, 0x24492929);	// writing MSLUT_2 @ address 27=0x62 with 0x24492929=608774441=0.0
  stepper.writeReg((TMC5130::Reg)0x63, 0x10104222);	// writing MSLUT_3 @ address 28=0x63 with 0x10104222=269500962=0.0
  stepper.writeReg((TMC5130::Reg)0x64, 0xFBFFFFFF);	// writing MSLUT_4 @ address 29=0x64 with 0xFBFFFFFF=0=0.0
  stepper.writeReg((TMC5130::Reg)0x65, 0xB5BB777D);	// writing MSLUT_5 @ address 30=0x65 with 0xB5BB777D=0=0.0
  stepper.writeReg((TMC5130::Reg)0x66, 0x49295556);	// writing MSLUT_6 @ address 31=0x66 with 0x49295556=1227445590=0.0
  stepper.writeReg((TMC5130::Reg)0x67, 0x00404222);	// writing MSLUT_7 @ address 32=0x67 with 0x00404222=4211234=0.0
  stepper.writeReg((TMC5130::Reg)0x68, 0xFFFF8056);	// writing MSLUTSEL @ address 33=0x68 with 0xFFFF8056=0=0.0
  stepper.writeReg((TMC5130::Reg)0x69, 0x00F70000);	// writing MSLUTSTART @ address 34=0x69 with 0x00F70000=16187392=0.0
  stepper.writeReg((TMC5130::Reg)0x6C, 0x000101D5);	// writing CHOPCONF @ address 35=0x6C with 0x000101D5=66005=0.0
  stepper.writeReg((TMC5130::Reg)0x6D, 0x00000000);	// writing COOLCONF @ address 36=0x6D with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x6E, 0x00000000);	// writing DCCTRL @ address 37=0x6E with 0x00000000=0=0.0
  stepper.writeReg((TMC5130::Reg)0x70, 0x000500C8);	// writing PWMCONF @ address 38=0x70 with 0x000500C8=327880=0.0
  stepper.writeReg((TMC5130::Reg)0x72, 0x00000000);	// writing ENCM_CTRL @ address 39=0x72 with 0x00000000=0=0.0
}

void SetFreeRunning(TMC5130 &stepper, uint8_t SpeedFor1RPS){ //
  stepper.setMicrosteps(0);
  stepper.setFirstAcceleration(1000);
  stepper.setSecondAcceleration(1000);
  stepper.setFirstDeceleration(1000);
  stepper.setMaxVelocity( (uint32_t)(53687*SpeedFor1RPS)>>(stepper.getMicrosteps()));
  stepper.setRampMode(TMC5130::TMC5130_MODE_VELPOS);
}