#include "TMC5130_Inits.h"

//From Datasheet:
//Enable the driver for step and direction operation and initialize the chopper for SpreadCycle operation and for StealthChop at <30 RPM:
void TMC5130_Init_DS0(TMC5130 &stepper) {
  stepper.setChopconf   (0x000100C3);  // CHOPCONF: TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle)
  stepper.setCurrent    (31, 10, 6);  //stepper.writeReg(TMC5130::IHOLD_IRUN, 0x00061F0A);  // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
  stepper.setTPowerDown (10);  // TPOWERDOWN=10: Delay before power down in stand still
  stepper.setGconf      (0x00000004);  //GCONF      EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
  stepper.setTPwmThrs   (500);  // TPWM_THRS=500 yields a switching velocity about 35000 = ca. 30RPM
  //stepper.writeReg(TMC5130::PWMCONF,    0x000401C8);  // PWMCONF: AUTO=1, 2/1024 Fclk, Switch amplitude limit=200, Grad=1
    { //0x000401C8 = 0 1 00 00000001 11001000
    TMC5130::Pwmconf pwmconf;         //PWMCONF
        pwmconf.pwm_ampl       = 200; //User defined maximum PWM amplitude
        pwmconf.pwm_grad       = 1;   //User defined maximum PWM amplitude change per half wave (1 to 15)
        pwmconf.pwm_freq       = 0;   //%00: fPWM=2/1024 fCLK
        pwmconf.pwm_autoscale  = 1;   //Enable automatic current control
        pwmconf.pwm_symmetric  = 0;
        pwmconf.freewheel      = 0;   //%00: Normal operation
    stepper.setPwmconf(pwmconf.bytes);
  }
}

//Enable and initialize the motion controller and move one rotation (51200 microsteps) using the ramp generator
void TMC5130_Init_DS1(TMC5130 &stepper) {
  stepper.setFirstAcceleration  (1000); // A1 = 1000 First acceleration
  stepper.setFirstVelocity      (50000); // V1 = 50000 Acceleration threshold velocity V1
  stepper.setSecondAcceleration (500); // AMAX = 500 Acceleration above V1
  stepper.setMaxVelocity        (200000); // VMAX = 200000
  stepper.setFirstDeceleration  (700); // DMAX = 700 Deceleration above V1
  stepper.setSecondDeceleration (1400); // D1 = 1400 Deceleration below V1
  stepper.setStopVelocity       (10); // VSTOP = 10 Stop velocity (Near to zero)
  stepper.setRampMode           (TMC5130::PositionMode); // RAMPMODE = 0 (Target position move)
  stepper.setPosition(0);
// Ready to move!
  stepper.setTarget(-51200); // XTARGET = -51200 = 0xFFFF3800 (Move one rotation left (200*256 microsteps)
// Now motor 1 starts rotating
//SPI send: 0x2100000000; // Query XACTUAL – The next read access delivers XACTUAL
//SPI read; // Read XACTUAL
}

//CURRENT SETTING AND FIRST STEPS WITH STEALTHCHOP
/*
//== Current Setting ==
//-- Check hardware setup and motor RMS current --
If (Sense Resistors NOT used) GCONF: set internal_Rsense
IF (Analog Scaling)           GCONF: set I_scale_analog
if(Low Current range)         CHOPCONF: set vsense for max. 180mV at sense resistor (0R15: 1.1A peak)

Set I_RUN as desired up to 31, I_HOLD 70% of I_RUN or lower
Set I_HOLD_DELAY to 1 to 15 for smooth standstill current decay
Set TPOWERDOWN up to 255 for delayed standstill current reduction
Configure Chopper to test current settings


//-- stealthChop Configuration --
GCONF: set en_pwm_mode
PWMCONF: set pwm_autoscale, set PWM_GRAD=1, PWM_AMPL=255
PWMCONF: select PWM_FREQ with regard to fCLK for about 35kHz PWM frequency
-- Make sure that no step pulses are generated --
CHOPCONF: Enable chopper using basic config.: TOFF=4, TBL=2, HSTART=4, HEND=0
-- Move the motor by slowly accelerating from 0 to VMAX operation velocity --
if(performance are NOT good up to VMAX) Select a velocity threshold for switching to spreadCycle chopper and set TPWMTHRS

//== MOVING THE MOTOR USING THE MOTION CONTROLLER ==
//-- Move Motor --
RAMPMODE: set velocity_positive
Set AMAX=1000, set VMAX=100000 or different values
Motor moves, change VMAX as desired

//-- Configure Ramp Parameters --

//Start Velocity
setStartVelocity(0);  //Set VSTART=0. Higher velcoity for abrupt start (limited by motor).
//Stop Velocity
setStopVelocity(10);  //Set VSTOP=10, but not below VSTART. Higher velocity for abrupt stop.
if (VSTOP IS relevant (>>10)){
  Set TZEROWAIT to allow motor to recover from jump VSTOP to 0, before going to VSTART
  Set TPOWERDOWN time not smaller than TZEROWAIT time. Min. value is TZEROWAIT/512
}
setFirstAcceleration(100);  //Set acceleration A1 as desired by application
setFirstVelocity(10000);    //Determine velocity, where max. motor torque or current sinks appreciably, write to V1
setMaxVelocity(20000);    //Set desired maximum velocity to VMAX
setSecondAcceleration(50);      //AMAX: Set lower acceleration than A1 to allow motor to accelerate up to VMAX
setFirstDeceleration(50);      //DMAX: Use same value as AMAX or higher
setSecondDeceleration(100);      //D1: Use same value as A1 or higher


//-- Move to Target --
setRampMode(PositionMode); //RAMPMODE: set position
Configure ramp parameters
Set XTARGET
*/

void TMC5130_Init_00(TMC5130 &stepper){
//1 Stop Motor
  stepper.setGconf              (0);        //writeReg(TMC5130::GCONF,       0);  0x00000004);  //SPI send: 0x8000000004; // EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
  stepper.setNodeConf           (0, 0);  //NODECONF
  stepper.setXCompare           (0);     //X_COMPARE
  stepper.setCurrent            (1, 1, 7);    //writeReg(TMC5130::IHOLD_IRUN,  0x00070101);  //  
  stepper.setTPowerDown         (0); //TPOWERDOWN
  stepper.setTPwmThrs           (0); //TPWMTHRS
  stepper.setTCOOLTHRS          (0);                //TCOOLTHRS
  stepper.setTHigh              (0);
  stepper.setRampMode           (TMC5130::VelocityNegativeMode);  //stepper.writeReg(TMC5130::RAMPMODE,    0x00000002);
  stepper.setPosition           (0xFFE38782);        //XACTUAL
  stepper.setStartVelocity      (0);            //VSTART
  stepper.setFirstAcceleration  (966);      //A1 0x000003C6
  stepper.setFirstVelocity      (107374);       //V1  0x0001A36E  
  stepper.setSecondAcceleration (966);    //AMAX 0x000003C6
  stepper.setMaxVelocity        (214748); //VMAX  0x000346DC
  stepper.setFirstDeceleration  (966);    //DMAX //0x000003C6
  stepper.setSecondDeceleration (966);    //D1 0x000003C6
  stepper.setStopVelocity       (10);     //VSTOP  0x0000000A

  stepper.setTZeroWait(0);    //TZEROWAIT
  stepper.setTarget(10000);  //XTARGET
  stepper.setVDCMin(0);       //VDCMIN
  
//  stepper.writeReg(TMC5130::SW_MODE,     0x00000000);
  {
    TMC5130::SwMode sw_mode = stepper.getSwMode();  //SW_MODE
    sw_mode.stop_l_enable    = 1;	   //1: Enables automatic motor stop during active left reference switch input
    sw_mode.stop_r_enable    = 1;    //1: Enables automatic motor stop during active right reference switch input
    sw_mode.pol_stop_l       = 1;    //0=non-inverted, high active, 1=inverted, low active
    sw_mode.pol_stop_r       = 0;    //0=non-inverted, high active, 1=inverted, low active
    sw_mode.swap_lr          = 0;    //1: Swap the left and the right reference switch input REFL and REFR
    sw_mode.latch_l_active   = 0;    //    
    sw_mode.latch_l_inactive = 0;    //  
    sw_mode.latch_r_active   = 0;    //    
    sw_mode.latch_r_inactive = 0;    //  
    sw_mode.en_latch_encoder = 0;    //  
    sw_mode.sg_stop          = 1;    //1: Enable stop by StallGuard2 (also available in DcStep mode). Disable to release motor after stop event.           
    sw_mode.en_softstop      = 1;    //0: Hard stop 1: Soft stop    
    
    stepper.setSwMode(sw_mode);
  }

  stepper.setEncMode( TMC5130::Encmode{.bytes=0} );  //ENCMODE
  stepper.setXEnc(0);     //X_ENC
  stepper.setEncConst( TMC5130::EncConst{.fractional=1, .integer=0} );//,   0x0001 0000);//ENC_CONST

  {
    TMC5130::Chopconf chopconf;
      chopconf.toff           = 5;  //off time and driver enable
      chopconf.HSTRT          = 5;  //hysteresis start value added to HEND
      chopconf.HEND           = 3;  //hysteresis low value OFFSET sine wave offset
      chopconf.fd3            = 0;  //
      chopconf.disfdcc        = 0;  //
      chopconf.rndtf          = 0;  //
      chopconf.chm            = 0;  //chopper mode. 0=Standard mode (SpreadCycle)
      chopconf.tbl            = 2;  //blank time select
      chopconf.vsense         = 0;  //
      chopconf.vhighfs        = 0;  //
      chopconf.vhighchm       = 0;  //
      chopconf.sync           = 0;  //
      chopconf.mres           = 0;  //micro step resolution 0=256
      chopconf.interpol       = 0;  //
      chopconf.dedge          = 0;  //
      chopconf.diss2g         = 0;  //
    stepper.setChopconf(chopconf.bytes);
  }

  stepper.setCoolconf (0);                      //COOLCONF
  stepper.setDcctrl   (0);                        //DCCTRL
  {
    TMC5130::Pwmconf pwmconf;         //PWMCONF
        pwmconf.pwm_ampl       = 128;//200; //default 128 User defined amplitude (offset)
        pwmconf.pwm_grad       = 4;//0;   //default 4   User defined amplitude (gradient) or regulation loop gradient
        pwmconf.pwm_freq       = 1;  //             PWM frequency selection
        pwmconf.pwm_autoscale  = 1;  // 0000        PWM automatic amplitude scaling
        pwmconf.pwm_symmetric  = 0;  //0x0008 0000  Force symmetric PWM
        pwmconf.freewheel      = 0;  //0x0030 0000  Allows different standstill modes TMC5130::StandstillMode.NormalMode
    stepper.setPwmconf(pwmconf.bytes);
  }

  //stepper.writeReg(TMC5130::ENCM_CTRL,   0x00000000);

  //Added for Stopping ALL
  stepper.StopMotor             (1000);
  stepper.setRampMode           (TMC5130::PositionMode);
  stepper.setPosition           (0);
  stepper.setTarget             (0);  //Include setRampMode
}

void TMC5130_Init_01(TMC5130 &stepper){
  stepper.setChopconf          (65731);  //writeReg(CHOPCONF,    0x000100C3);  //SPI send: 0xEC000100C3; // CHOPCONF: TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle)

//  irun      = 0x1F; //31  Corrente a motore in moto 0...31
//  ihold     = 0xA;  //10  Corrente a motore fermo 0...31
//  holdDelay = 0x06; //6   0: spegnimento istantaneo, 1..15: Ritardo per ogni step di riduzione della corrente in multipli di 2^18 clock
  stepper.setCurrent    (31, 10, 6);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
  stepper.setTPowerDown (10);  //SPI send: 0x910000000A; // TPOWERDOWN=10: Delay before power down in stand still
  stepper.setGconf      (0x00000004);  //SPI send: 0x8000000004; // EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
  stepper.setTPwmThrs   (500);  //SPI send: 0x93000001F4; // TPWM_THRS=500 yields a switching velocity about 35000 = ca. 30RPM
  stepper.setPwmconf    (0x000401C8);  //SPI send: 0xF0000401C8;

  stepper.setFirstAcceleration  (1000);                   //writeReg(A1,          0x000003E8);  //SPI send: 0xA4000003E8; // A1 = 1000 First acceleration
  stepper.setFirstVelocity      (50000);                  //writeReg(V1,          0x0000C350);  //SPI send: 0xA50000C350; // V1 = 50000 Acceleration threshold velocity V1
  stepper.setSecondAcceleration (500);                    //writeReg(AMAX,        0x000001F4);  //SPI send: 0xA6000001F4; // AMAX = 500 Acceleration above V1
  stepper.setMaxVelocity        (200000);                 //writeReg(VMAX,        0x00030D40);  //SPI send: 0xA700030D40; // VMAX = 200000
  stepper.setFirstDeceleration  (700);                    //writeReg(DMAX,        0x000002BC);  //SPI send: 0xA8000002BC; // DMAX = 700 Deceleration above V1
  stepper.setSecondDeceleration (1400);                   //writeReg(D1,          0x00000578);  //SPI send: 0xAA00000578; // D1 = 1400 Deceleration below V1
  stepper.setStopVelocity       (10);                     //writeReg(VSTOP,       0x0000000A);  //SPI send: 0xAB0000000A; // VSTOP = 10 Stop velocity (Near to zero)
  stepper.setRampMode           (TMC5130::PositionMode);  //writeReg(RAMPMODE,    0x00000000);  //SPI send: 0xA000000000; // RAMPMODE = 0 (Target position move)

#define ENABLE_STALLGUARD

  TMC5130::SwMode sw_mode = stepper.getSwMode();  //SW_MODE
      sw_mode.stop_l_enable		  = 1;
      sw_mode.stop_r_enable		  = 0;
      sw_mode.pol_stop_l		    = 1;
      sw_mode.pol_stop_r		    = 0;
      sw_mode.swap_lr			      = 0;
      sw_mode.latch_l_active	  = 0;
      sw_mode.latch_l_inactive	= 0;
      sw_mode.latch_r_active	  = 0;
      sw_mode.latch_r_inactive	= 0;
      sw_mode.en_latch_encoder	= 0;
    #if defined(ENABLE_STALLGUARD)
      sw_mode.sg_stop			      = 1;
    #else
      sw_mode.sg_stop			      = 0;
    #endif
      sw_mode.en_softstop		    = 0;

  stepper.setSwMode(sw_mode);

/*
//  stepper.StopEnable(true);
  stepper.SetSwMode((TMC5130::SwModes)(
      0
      | TMC5130::SW_MODE_STOP_L_ENABLE | TMC5130::SW_MODE_POL_STOP_L  //Enable Left Limit Switch
  //  | TMC5130::SW_MODE_STOP_R_ENABLE
  //  | TMC5130::SW_MODE_POL_STOP_R
  //  | TMC5130::SW_MODE_SWAP_LR
#if defined(ENABLE_STALLGUARD)
      | TMC5130::SW_MODE_SG_STOP     //Enable StallGuard2
#endif
  )
  );
*/
  stepper.setCurrent(1, 1, 1);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
  stepper.setMicrosteps(8);
#if defined(ENABLE_STALLGUARD)
  stepper.setTCOOLTHRS(20);
#endif
}

void TMC5130_Init_02(TMC5130 &stepper, bool Start){
  stepper.setGconf    (0);	      // writing GCONF @ address 0=0x00 with 0x00000000=0=0.0
  stepper.setNodeConf (0, 0);     //ToDo (TMC5130::Reg)0x03, 0x00000000);	// writing SLAVECONF @ address 1=0x03 with 0x00000000=0=0.0
  stepper.setXCompare (0);	      // writing X_COMPARE @ address 2=0x05 with 0x00000000=0=0.0
  stepper.setCurrent  (3, 23, 7); //stepper.writeReg((TMC5130::Reg)0x10, 0x00071703);	// writing IHOLD_IRUN @ address 3=0x10 with 0x00071703=464643=0.0

  stepper.setTPowerDown (0);	// writing TPOWERDOWN @ address 4=0x11 with 0x00000000=0=0.0
  stepper.setTPwmThrs   (0);	// writing TPWMTHRS @ address 5=0x13 with 0x00000000=0=0.0
  stepper.setTCOOLTHRS  (0);	// writing TCOOLTHRS @ address 6=0x14 with 0x00000000=0=0.0
  stepper.setTHigh      (0);	// writing THIGH @ address 7=0x15 with 0x00000000=0=0.0

  if(Start){
    stepper.setRampMode   (TMC5130::VelocityPositiveMode); //    stepper.writeReg((TMC5130::Reg)0x20, 0x00000001);	// writing RAMPMODE @ address 8=0x20 with 0x00000001=1=0.0
    stepper.setPosition   (590271);	// writing XACTUAL  @ address 9=0x21 with 0x000901BF=590271=0.0
    stepper.setMaxVelocity(53687);            //stepper.writeReg((TMC5130::Reg)0x27, 0x0000D1B7);	// writing VMAX     @ address 14=0x27 with 0x0000D1B7=53687=0.0
  }else{
    stepper.setRampMode         (TMC5130::PositionMode);	// writing RAMPMODE @ address 8=0x20 with 0x00000000=0=0.0
    stepper.setPosition         (0);                       // writing XACTUAL  @ address 9=0x21 with 0x00000000=0=0.0
    stepper.setMaxVelocity      (0);                    // writing VMAX     @ address 14=0x27 with 0x00000000=0=0.0
  }
  stepper.setStartVelocity      (0);	// writing VSTART @ address 10=0x23 with 0x00000000=0=0.0
  stepper.setFirstAcceleration  (0);	// writing A1 @ address 11=0x24 with 0x00000000=0=0.0
  stepper.setFirstVelocity      (0);	// writing V1 @ address 12=0x25 with 0x00000000=0=0.0
  stepper.setSecondAcceleration (100); // stepper.writeReg((TMC5130::Reg)0x26, 0x00000064);	// writing AMAX @ address 13=0x26 with 0x00000064=100=0.0
  stepper.setFirstDeceleration  (100);  //stepper.writeReg((TMC5130::Reg)0x28, 0x00000064);	// writing DMAX @ address 15=0x28 with 0x00000064=100=0.0
  stepper.setSecondDeceleration (0);	// writing D1 @ address 16=0x2A with 0x00000000=0=0.0
  stepper.setStopVelocity       (0);	// writing VSTOP @ address 17=0x2B with 0x00000000=0=0.0
  stepper.setTZeroWait          (0);	// writing TZEROWAIT @ address 18=0x2C with 0x00000000=0=0.0
  stepper.setTarget             (0);	// writing XTARGET @ address 19=0x2D with 0x00000000=0=0.0
  stepper.setVDCMin              (0);	// writing VDCMIN @ address 20=0x33 with 0x00000000=0=0.0
  stepper.setSwMode             ( TMC5130::SwMode{.bytes=0} );	// writing SW_MODE @ address 21=0x34 with 0x00000000=0=0.0
  stepper.setEncMode            ( TMC5130::Encmode{.bytes=0} );	// writing ENCMODE @ address 22=0x38 with 0x00000000=0=0.0
  stepper.setXEnc               (0);	// writing X_ENC @ address 23=0x39 with 0x00000000=0=0.0
  stepper.setEncConst           ( TMC5130::EncConst{.fractional=1, .integer=0} );	// writing ENC_CONST @ address 24=0x3A with 0x00010000=65536=0.0
/*
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
*/
  stepper.setChopconf ( 0x000101D5);	// writing CHOPCONF @ address 35=0x6C with 0x000101D5=66005=0.0
  stepper.setCoolconf (0);           //writeReg((TMC5130::Reg)0x6D, 0x00000000);	// writing COOLCONF @ address 36=0x6D with 0x00000000=0=0.0
  stepper.setDcctrl   (0x00000000);	  // writing DCCTRL @ address 37=0x6E with 0x00000000=0=0.0
  stepper.setPwmconf  (0x000500C8);	// ToDo: Maskera di bit, usare Pwmconf  ..... writing PWMCONF @ address 38=0x70 with 0x000500C8=327880=0.0
  stepper.setEncmCrtl ( TMC5130::EncmCrtl{.bytes=0} );	// writing ENCM_CTRL @ address 39=0x72 with 0x00000000=0=0.0
}

void SetFreeRunning(TMC5130 &stepper, uint8_t SpeedFor1RPS, uint8_t mres){ //
  stepper.setMicrosteps(mres);
  stepper.setFirstAcceleration(1000);
  stepper.setSecondAcceleration(1000);
  stepper.setFirstDeceleration(1000);
  stepper.setMaxVelocity( (uint32_t)(53687*SpeedFor1RPS)>>(stepper.getMicrosteps()));
  stepper.setRampMode(TMC5130::VelocityPositiveMode);
}

void SetPositional(TMC5130 &stepper, uint8_t SpeedFor1RPS, uint8_t mres){ //
  stepper.StopMotor             (1000);
  stepper.setMicrosteps         (mres);
  stepper.setFirstAcceleration  (100);
  stepper.setSecondAcceleration (100);
  stepper.setFirstDeceleration  (100);
  stepper.setMaxVelocity        ( (uint32_t)(53687*SpeedFor1RPS)>>(stepper.getMicrosteps()));
  stepper.setRampMode           (TMC5130::PositionMode);
  stepper.setPosition           (0);
  stepper.setTarget             (0);  //Include setRampMode
}

void enableStealthChop(TMC5130 &stepper, bool En) {  //GCONF  //True=enable, False=disable
  TMC5130::Gconf gconf = stepper.getGconf(); //GCONF
  gconf.en_pwm_mode = En?1:0;
  stepper.setGconf(gconf.bytes);
}

void enableStallGuardFilter(TMC5130 &stepper, bool En) {  //COOLCONF  //True=enable, False=disable
  TMC5130::Coolconf coolconf = stepper.getCoolconf();
  coolconf.sfilt = En?1:0;
  stepper.setCoolconf(coolconf.bytes);
}

void enableCoolStep(TMC5130 &stepper, uint8_t min, uint8_t max) { //COOLCONF
  TMC5130::Coolconf coolconf = stepper.getCoolconf();
  coolconf.semin = min;
  coolconf.semax = max;
  stepper.setCoolconf(coolconf.bytes);
}

void disableCoolStep(TMC5130 &stepper) {  //COOLCONF
  TMC5130::Coolconf coolconf = stepper.getCoolconf();
  coolconf.semin = 0; //SEMIN_OFF;
  stepper.setCoolconf(coolconf.bytes);
}

void writeStallGuardThreshold(TMC5130 &stepper, int8_t threshold) { //COOLCONF
  TMC5130::Coolconf coolconf = stepper.getCoolconf();
  coolconf.sgt = threshold;
  stepper.setCoolconf(coolconf.bytes);
}

void writeDcTime(TMC5130 &stepper, uint16_t dc_time) {  //DCCTRL
  TMC5130::Dcctrl dcctrl;
  dcctrl.bytes = stepper.getDcctrl();
  dcctrl.dc_time = dc_time;
  stepper.setDcctrl(dcctrl.bytes);
}

void writeDcStallGuardThreshold(TMC5130 &stepper, uint8_t dc_stall_guard_threshold) { //DCCTRL
  TMC5130::Dcctrl dcctrl;
  dcctrl.bytes = stepper.getDcctrl();
  dcctrl.dc_sg = dc_stall_guard_threshold;
  stepper.setDcctrl(dcctrl.bytes);
}

void writePwmOffset(TMC5130 &stepper, uint8_t pwm_amplitude) {  //PWMCONF
  TMC5130::Pwmconf pwmconf;
  pwmconf.bytes = stepper.getPwmconf();
  pwmconf.pwm_ampl = pwm_amplitude;    //pwm_ampl alias pwm_ofs
  stepper.setPwmconf(pwmconf.bytes);
}

void writePwmGradient(TMC5130 &stepper, uint8_t pwm_amplitude) {  //PWMCONF
  TMC5130::Pwmconf pwmconf;
  pwmconf.bytes = stepper.getPwmconf();
  pwmconf.pwm_grad = pwm_amplitude;
  stepper.setPwmconf(pwmconf.bytes);
}

void enableAutomaticCurrentControl(TMC5130 &stepper, bool en) {  //PWMCONF
  TMC5130::Pwmconf pwmconf;
  pwmconf.bytes = stepper.getPwmconf();
  pwmconf.pwm_autoscale = en?1:0;
  pwmconf.pwm_symmetric = en?1:0; //alias pwm_autograd
//pwmconf.pwm_reg = pwm_reg;      //pwm_reg does not exists in 5130
  stepper.setPwmconf(pwmconf.bytes);
}





uint8_t readVersion(TMC5130 &stepper) { //IOIN
  return stepper.getIoin().version;
}


void InitTestStall(TMC5130 &stepper){
/*
    --driver_parameters_real:--                                                     
    run_current...........%: 50 % di 31                                                    
    hold_current..........%: 10 % di 31                                      
    pwm_offset............%: 30 % di 255                                 
    pwm_gradient..........%: 10 % di 255
    stealth_chop_threshold.:100 radians/s 

int32_t Converter::velocityHzToTstep(int32_t velocity_hz) {
  if (velocity_hz == 0)  velocity_hz = 1;
  int64_t tstep = ((int64_t)clock_frequency_mhz * 1000000) / velocity_hz;
  return tstep;
}

int32_t Converter::velocityRealToTstep(int32_t velocity_real) {
  return velocityHzToTstep(velocityRealToHz(velocity_real));
}


DriverParameters Converter::driverParametersRealToChip(DriverParameters parameters) {
  DriverParameters parameters_chip = parameters;
  parameters_chip.global_current_scaler   = percentToGlobalCurrentScaler  (parameters.global_current_scaler);
  parameters_chip.run_current             = percentToCurrentSetting       (parameters.run_current);
  parameters_chip.hold_current            = percentToCurrentSetting       (parameters.hold_current);
  parameters_chip.hold_delay              = percentToHoldDelaySetting     (parameters.hold_delay);
  parameters_chip.pwm_offset              = percentToPwmSetting           (parameters.pwm_offset);
  parameters_chip.pwm_gradient            = percentToPwmSetting           (parameters.pwm_gradient);
  parameters_chip.stealth_chop_threshold  = velocityRealToTstep           (parameters.stealth_chop_threshold);
  parameters_chip.cool_step_threshold     = velocityRealToTstep           (parameters.cool_step_threshold);
  parameters_chip.high_velocity_threshold = velocityRealToTstep           (parameters.high_velocity_threshold);

  return parameters_chip;
}

*/

  //--driver_parameters_chip:--
  //registers_ptr_->write(Registers::GlobalScalerAddress, scaler);  //global_current_scaler................: 0
  stepper.setCurrent(15, 3, 0); //IHOLD_IRUN  //run, hold, delay

  TMC5130::Pwmconf x;
    x.bytes = stepper.getPwmconf();
    x.pwm_ampl      = 0x4C;   //pwm_offset...........................: 76  //PWMCONF
    x.pwm_grad      = 0x19;   //pwm_gradient.........................: 25  //PWMCONF
    x.pwm_autoscale = 0;      //automatic_current_control_enabled....: 0   //PWMCONF
    stepper.setPwmconf(x.bytes);

  stepper.setMotorDirection(TMC5130::ForwardDirection);     //motor_direction......................: 0  //GCONF
  stepper.writeStandstill_Normal  ();        //standstill_mode......................: 0  //PWMCONF
  stepper.writeChopperMode   (TMC5130::SpreadCycleMode);    //chopper_mode.........................: 0  //CHOPCONF
  stepper.setTPwmThrs   (14);              //stealth_chop_threshold...............: 14 //TPWMTHRS
  enableStealthChop     (stepper, true);                       //stealth_chop_enabled.................: 1  //GCONF
  stepper.setTCOOLTHRS  (0x9);             //cool_step_threshold..................: 9  //TCOOLTHRS
  enableCoolStep        (stepper, 1, 0);                       //cool_step_min........................: 1  //COOLCONF
                                                            //cool_step_max........................: 0  //COOLCONF
                                                            //cool_step_enabled....................: 0  ???
  stepper.setTHigh                    (7);    //high_velocity_threshold..............: 7  //THIGH
  stepper.enableHighVelocityFullstep  (false);         //high_velocity_fullstep_enabled.......: 0  //CHOPCONF
  stepper.enableHighVelocityChopperSwitch (false);         //high_velocity_chopper_switch_enabled.: 0  //CHOPCONF
  writeStallGuardThreshold        (stepper, 0);             //stall_guard_threshold................: 0  //COOLCONF
  enableStallGuardFilter          (stepper, false);         //stall_guard_filter_enabled...........: 0  //COOLCONF
  stepper.enableShortToGroundProtection   (false);         //short_to_ground_protection_enabled...: 1 (Inverted) //CHOPCONF  //True=enable=0, False=disable=1
  stepper.writeEnabledToff                (3);             //enabled_toff.........................: 3  //CHOPCONF
  stepper.writeComparatorBlankTime(TMC5130::ClockCycles36);   //comparator_blank_time................: 2 //CHOPCONF
  writeDcTime                     (stepper, 0);             //dc_time..............................: 0  //DCCTRL
  writeDcStallGuardThreshold      (stepper, 0);             //dc_stall_guard_threshold.............: 0  //DCCTRL

/*
  --controller_parameters_real:--                                                 
  ramp_mode........:0                                                             
  max_velocity.....:20 radians/s                                                  
  max_acceleration.:2 radians/s)/s                                                
*/
  stepper.setRampMode             (TMC5130::PositionMode);  //ramp_mode............: 0        //RAMPMODE
  stepper.writeStopMode           (TMC5130::HardMode);      //stop_mode............: 0        //SW_MODE
  stepper.setMaxVelocity          (227862);                 //max_velocity.........: 227862   //VMAX
  stepper.setSecondAcceleration   (248);                    //max_acceleration.....: 248      //AMAX
  stepper.setStartVelocity        (11393);                  //start_velocity.......: 11393    //VSTART
  stepper.setStopVelocity         (113931);                 //stop_velocity........: 113931   //VSTOP
  stepper.setFirstVelocity        (0);                      //first_velocity.......: 0        //V1
  stepper.setFirstAcceleration    (0);                      //first_acceleration...: 0        //A1
  stepper.setFirstDeceleration    (0);                      //max_deceleration.....: 0        //DMAX
  stepper.setSecondDeceleration   (1244);                   //first_deceleration...: 1244     //D1
  stepper.setTZeroWait            (0);                      //zero_wait_duration...: 0        //TZEROWAIT
  stepper.enableStallStop         (false);                  //stall_stop_enabled...: 0        //SW_MODE
  stepper.setVDCMin               (0);                      //min_dc_step_velocity.: 0        //VDCMIN

/*
--home_parameters_real:--                                                       
run_current.....: 25 %
hold_current....: 10 %
target_position.: -100 radians
velocity........: 20 radians/s
acceleration....: 2 radians/s)/s
*/
//  stepper.setCurrent(7, 3, ????); //IHOLD_IRUN  //run, hold, delay
  stepper.SetIrun(7);                                         //run_current.........: 7         //IHOLD_IRUN
  stepper.SetIhold(3);                                         //hold_current........: 3         //IHOLD_IRUN

  stepper.setTarget               (-814900);             //target_position.....: -814900    //XTARGET
  stepper.setFirstVelocity        (227862);                                            //velocity............: 227862  //??????????????????????
  stepper.setFirstAcceleration    (248);                                               //acceleration........: 248     //??????????????????????????????
  stepper.setTZeroWait            (2353);                         //zero_wait_duration..: 2353  //TZEROWAIT

/*
--stall_parameters_real:--
stall_guard_threshold.: 3 64..63, 0 default, higher is less sensitive
cool_step_threshold...: 10 radians/s
*/
  writeStallGuardThreshold      (stepper, 3);     //stall_guard_threshold.: 3   //COOLCONF
  stepper.setTCOOLTHRS          (147);            //cool_step_threshold...: 147 //TCOOLTHRS 
}

bool communicating(TMC5130 &stepper) {
  uint8_t version = stepper.getIcVersion();
  //uint8_t version = readVersion();
  return ((version == 0x11)   //Registers::VERSION_TMC5130)
       || (version == 0x30)); //Registers::VERSION_TMC5160));
}




void writeDriverParameters(TMC5130 &stepper, TMC5130::DriverParameters parameters) { //stepper.driver.setup(driver_parameters_chip);
  //Non esiste  writeGlobalCurrentScaler(parameters.global_current_scaler);
  stepper.setCurrent(parameters.run_current, parameters.hold_current, parameters.hold_delay); //IHOLD_IRUN  //run, hold, delay
  writePwmOffset                (stepper, parameters.pwm_offset);   //PWMCONF
  writePwmGradient              (stepper, parameters.pwm_gradient); //PWMCONF
  enableAutomaticCurrentControl (stepper, parameters.automatic_current_control_enabled);  //PWMCONF
  stepper.setMotorDirection     (parameters.motor_direction);  //GCONF  True=1=ReverseDirection, false=0=ForwardDirection
  stepper.writeStandstillMode   (parameters.standstill_mode); //PWMCONF
  stepper.writeChopperMode      (parameters.chopper_mode);  //CHOPCONF
  stepper.setTPwmThrs           (parameters.stealth_chop_threshold);  //TPWMTHRS  writeStealthChopThreshold
  enableStealthChop             (stepper, parameters.stealth_chop_enabled);  //GCONF  //True=enable, False=disable
  stepper.setTCOOLTHRS          (parameters.cool_step_threshold);  //TCOOLTHRS  writeCoolStepThreshold
  if (parameters.cool_step_enabled)
    enableCoolStep(stepper, parameters.cool_step_min, parameters.cool_step_max); //COOLCONF
  else
    disableCoolStep(stepper);  //COOLCONF
  stepper.setTHigh                    (parameters.high_velocity_threshold);     //THIGH  writeHighVelocityThreshold
  stepper.enableHighVelocityFullstep  (parameters.high_velocity_fullstep_enabled);       //CHOPCONF //True=enable, False=disable
  stepper.enableHighVelocityChopperSwitch (parameters.high_velocity_chopper_switch_enabled); //CHOPCONF //True=enable, False=disable
  writeStallGuardThreshold        (stepper, parameters.stall_guard_threshold);                //COOLCONF
  enableStallGuardFilter          (stepper, parameters.stall_guard_filter_enabled);           //COOLCONF  //True=enable, False=disable
  stepper.enableShortToGroundProtection   (parameters.short_to_ground_protection_enabled);   //CHOPCONF  //True=enable=0, False=disable=1
  stepper.writeEnabledToff                (parameters.enabled_toff);                         //CHOPCONF
  stepper.writeComparatorBlankTime(parameters.comparator_blank_time);                //CHOPCONF
  writeDcTime                     (stepper, parameters.dc_time);                              //DCCTRL
  writeDcStallGuardThreshold      (stepper, parameters.dc_stall_guard_threshold);             //DCCTRL
}

void writeControllerParameters(TMC5130 &stepper, TMC5130::ControllerParameters parameters) {
  stepper.setRampMode           (parameters.ramp_mode);            //RAMPMODE
  stepper.writeStopMode         (parameters.stop_mode);            //SW_MODE
  stepper.setMaxVelocity        (parameters.max_velocity);         //VMAX
  stepper.setSecondAcceleration (parameters.max_acceleration);     //AMAX
  stepper.setStartVelocity      (parameters.start_velocity);       //VSTART
  stepper.setStopVelocity       (parameters.stop_velocity);        //VSTOP
  stepper.setFirstVelocity      (parameters.first_velocity);       //V1
  stepper.setFirstAcceleration  (parameters.first_acceleration);   //A1
  stepper.setFirstDeceleration  (parameters.max_deceleration);     //DMAX
  stepper.setSecondDeceleration (parameters.first_deceleration);   //D1
  stepper.setTZeroWait          (parameters.zero_wait_duration);   //TZEROWAIT
  stepper.enableStallStop       (parameters.stall_stop_enabled);   //SW_MODE
  stepper.setVDCMin             (parameters.min_dc_step_velocity); //TZEROWAIT
  
}

void InitTestStall_Setup(TMC5130 &stepper){
  TMC5130::ConverterParameters  converter_parameters;
    converter_parameters.clock_frequency_mhz                = 12;
    converter_parameters.microsteps_per_real_position_unit  = 8149;
    converter_parameters.seconds_per_real_velocity_unit     = 1;

  TMC5130::DriverParameters driver_parameters_chip;
    //driver_parameters_chip.global_current_scaler                    = 0;
    driver_parameters_chip.run_current                              = 15;
    driver_parameters_chip.hold_current                             = 3;
    driver_parameters_chip.hold_delay                               = 0;
    driver_parameters_chip.pwm_offset                               = 76;
    driver_parameters_chip.pwm_gradient                             = 25;
    driver_parameters_chip.automatic_current_control_enabled        = false;
    driver_parameters_chip.motor_direction                          = TMC5130::ForwardDirection;// (0);
    driver_parameters_chip.standstill_mode                          = TMC5130::NormalMode;// (0);
    driver_parameters_chip.chopper_mode                             = TMC5130::SpreadCycleMode;// (0);
    driver_parameters_chip.stealth_chop_threshold                   = 14;
    driver_parameters_chip.stealth_chop_enabled                     = true;
    driver_parameters_chip.cool_step_threshold                      = 9;
    driver_parameters_chip.cool_step_min                            = 1;
    driver_parameters_chip.cool_step_max                            = 0;
    driver_parameters_chip.cool_step_enabled                        = false;
    driver_parameters_chip.high_velocity_threshold                  = 7;
    driver_parameters_chip.high_velocity_fullstep_enabled           = false;
    driver_parameters_chip.high_velocity_chopper_switch_enabled     = false;
    driver_parameters_chip.stall_guard_threshold                    = 0;
    driver_parameters_chip.stall_guard_filter_enabled               = false;
    driver_parameters_chip.short_to_ground_protection_enabled       = true;
    driver_parameters_chip.enabled_toff                             = 3;
    driver_parameters_chip.comparator_blank_time                    = TMC5130::ClockCycles36;// (2)
    driver_parameters_chip.dc_time                                  = 0;
    driver_parameters_chip.dc_stall_guard_threshold                 = 0;

  TMC5130::ControllerParameters controller_parameters_chip;
    controller_parameters_chip.ramp_mode                = TMC5130::PositionMode;// (0)
    controller_parameters_chip.stop_mode                = TMC5130::HardMode;// (0)
    controller_parameters_chip.max_velocity             = 227862;
    controller_parameters_chip.max_acceleration         = 248;
    controller_parameters_chip.start_velocity           = 11393;
    controller_parameters_chip.stop_velocity            = 113931;
    controller_parameters_chip.first_velocity           = 0;
    controller_parameters_chip.first_acceleration       = 0;
    controller_parameters_chip.max_deceleration         = 0;
    controller_parameters_chip.first_deceleration       = 1244;
    controller_parameters_chip.zero_wait_duration       = 0;
    controller_parameters_chip.stall_stop_enabled       = false;
    controller_parameters_chip.min_dc_step_velocity     = 0;    
/* 
  TMC5130::HomeParameters       home_parameters_chip;
    home_parameters_chip.run_current        = 7;
    home_parameters_chip.hold_current       = 3;
    home_parameters_chip.target_position    = 4294152396;
    home_parameters_chip.velocity           = 227862;
    home_parameters_chip.acceleration       = 248;
    home_parameters_chip.zero_wait_duration = 2353;

  TMC5130::StallParameters      stall_parameters_chip;
    stall_parameters_chip.stall_guard_threshold = 3;
    stall_parameters_chip.cool_step_threshold   = 147;
*/
  writeDriverParameters     (stepper, driver_parameters_chip);      //stepper.driver.setup(driver_parameters_chip);
  writeControllerParameters (stepper, controller_parameters_chip);  //stepper.controller.setup(controller_parameters_chip);

  while (!stepper.IsConnected()) {
    Serial.println("No communication detected, check motor power and connections.");
    delay(500);
  }

  while (stepper.stepAndDirectionMode()) {
    Serial.println("Step and Direction mode enabled so SPI/UART motion commands will not work!");
    delay(500);
  }

}


void cacheDriverSettings(TMC5130::DriverParameters cached_driver_settings_) { //ToDo
/*
  cached_driver_settings_.global_current_scaler = registers_ptr_->getStored(Registers::GlobalScalerAddress);

  stepper.setCurrent(ihold_irun.irun, ihold_irun.ihold, ihold_irun.iholddelay); //IHOLD_IRUN  //run, hold, delay

  Registers::Pwmconf pwmconf;
    pwmconf.bytes = registers_ptr_->getStored(Registers::PwmconfAddress); //AAA readonly!!!!
    cached_driver_settings_.pwm_offset                        = pwmconf.pwm_ofs;
    cached_driver_settings_.pwm_gradient                      = pwmconf.pwm_grad;
    cached_driver_settings_.automatic_current_control_enabled = pwmconf.pwm_autoscale;

  Registers::Gconf gconf;
    gconf.bytes = registers_ptr_->getStored(Registers::GconfAddress);
    cached_driver_settings_.motor_direction = (MotorDirection)gconf.shaft;
    cached_driver_settings_.standstill_mode = (StandstillMode)pwmconf.freewheel;

  Registers::Chopconf chopconf;
    chopconf.bytes = registers_ptr_->getStored(Registers::ChopconfAddress);
    cached_driver_settings_.chopper_mode            = (ChopperMode)chopconf.chm;
    cached_driver_settings_.stealth_chop_threshold  = registers_ptr_->getStored(Registers::TpwmthrsAddress);
    cached_driver_settings_.stealth_chop_enabled    = gconf.en_pwm_mode;
    cached_driver_settings_.cool_step_threshold     = registers_ptr_->getStored(Registers::TcoolthrsAddress);

  Registers::Coolconf coolconf;
    coolconf.bytes = registers_ptr_->getStored(Registers::CoolconfAddress);
    cached_driver_settings_.cool_step_min                         = coolconf.semin;
    cached_driver_settings_.cool_step_max                         = coolconf.semax;
    cached_driver_settings_.cool_step_enabled                     = not (coolconf.semin == SEMIN_OFF);
    cached_driver_settings_.high_velocity_threshold               = registers_ptr_->getStored(Registers::ThighAddress);
    cached_driver_settings_.high_velocity_fullstep_enabled        = chopconf.vhighfs;
    cached_driver_settings_.high_velocity_chopper_switch_enabled  = chopconf.vhighchm;
    cached_driver_settings_.stall_guard_threshold                 = coolconf.sgt;
    cached_driver_settings_.stall_guard_filter_enabled            = coolconf.sfilt;
    cached_driver_settings_.short_to_ground_protection_enabled    = chopconf.diss2g;
    cached_driver_settings_.enabled_toff                          = enabled_toff_;
    cached_driver_settings_.comparator_blank_time                 = (ComparatorBlankTime)chopconf.tbl;

  Registers::Dcctrl dcctrl;
    dcctrl.bytes = registers_ptr_->getStored(Registers::DcctrlAddress);
    cached_driver_settings_.dc_time                   = dcctrl.dc_time;
    cached_driver_settings_.dc_stall_guard_threshold  = dcctrl.dc_sg;
*/  
}

void cacheControllerSettings() {  //ToDo
/*
  cached_controller_settings_.ramp_mode             = (RampMode)registers_ptr_->getStored(Registers::RampmodeAddress);
  Registers::SwMode sw_mode; sw_mode.bytes          = registers_ptr_->getStored(Registers::SwModeAddress);
  cached_controller_settings_.stop_mode             = (StopMode)sw_mode.en_softstop;
  cached_controller_settings_.max_velocity          = registers_ptr_->getStored(Registers::VmaxAddress);
  cached_controller_settings_.max_acceleration      = registers_ptr_->getStored(Registers::AmaxAddress);
  cached_controller_settings_.start_velocity        = registers_ptr_->getStored(Registers::VstartAddress);
  cached_controller_settings_.stop_velocity         = registers_ptr_->getStored(Registers::VstopAddress);
  cached_controller_settings_.first_velocity        = registers_ptr_->getStored(Registers::Velocity1Address);
  cached_controller_settings_.first_acceleration    = registers_ptr_->getStored(Registers::Acceleration1Address);
  cached_controller_settings_.max_deceleration      = registers_ptr_->getStored(Registers::DmaxAddress);
  cached_controller_settings_.first_deceleration    = registers_ptr_->getStored(Registers::Deceleration1Address);
  cached_controller_settings_.zero_wait_duration    = registers_ptr_->getStored(Registers::TzerowaitAddress);
  cached_controller_settings_.stall_stop_enabled    = sw_mode.sg_stop;
  cached_controller_settings_.min_dc_step_velocity = registers_ptr_->getStored(Registers::VdcminAddress);
*/  
}

void cacheSwitchSettings(){
/*
  Registers::SwMode sw_mode;
  sw_mode.bytes = registers_ptr_->getStored(Registers::SwModeAddress);
  cached_switch_settings_.left_stop_enabled     = sw_mode.stop_l_enable;
  cached_switch_settings_.right_stop_enabled    = sw_mode.stop_r_enable;
  cached_switch_settings_.invert_left_polarity  = sw_mode.pol_stop_l;
  cached_switch_settings_.invert_right_polarity = sw_mode.pol_stop_r;
  cached_switch_settings_.swap_left_right       = sw_mode.swap_lr;
  cached_switch_settings_.latch_left_active     = sw_mode.latch_l_active;
  cached_switch_settings_.latch_left_inactive   = sw_mode.latch_l_inactive;
  cached_switch_settings_.latch_right_active    = sw_mode.latch_r_active;
  cached_switch_settings_.latch_right_inactive  = sw_mode.latch_r_inactive;
  cached_switch_settings_.latch_encoder_enabled = sw_mode.en_latch_encoder;
*/  
}



void beginHomeToStall(TMC5130 &stepper, TMC5130::HomeParameters home_parameters, TMC5130::StallParameters stall_parameters) {
//  driver.cacheDriverSettings(); //ToDo
    cacheControllerSettings();  //ToDo
    cacheSwitchSettings();  //ToDo
  TMC5130::DriverParameters driver_parameters;
  writeDriverParameters(stepper, driver_parameters); //driver.setup(driver_parameters);

  TMC5130::ControllerParameters controller_parameters;
  writeControllerParameters (stepper, controller_parameters);                   //controller.setup(controller_parameters);
  stepper.writeStopMode           (TMC5130::HardMode);                          //controller.writeStopMode(HardMode);
  stepper.enableStallStop         (true);                                       //controller.enableStallStop();
  enableStallGuardFilter    (stepper, false);                                   //driver.disableStallGuardFilter();
  writeStallGuardThreshold  (stepper, stall_parameters.stall_guard_threshold);  //driver.writeStallGuardThreshold(stall_parameters.stall_guard_threshold);

  stepper.setCurrent            (home_parameters.run_current, home_parameters.hold_current, 0); //IHOLD_IRUN  //run, hold, delay
  stepper.writeStandstillMode   (TMC5130::NormalMode);                      //driver.writeStandstillMode(NormalMode);
  stepper.setRampMode           (TMC5130::HoldMode);                        //RAMPMODE  controller.writeRampMode(HoldMode);
  enableStealthChop             (stepper, false);                           //driver.disableStealthChop();
  disableCoolStep               (stepper);                                  //driver.disableCoolStep();
  stepper.setTCOOLTHRS          (stall_parameters.cool_step_threshold);     //driver.writeCoolStepThreshold(stall_parameters.cool_step_threshold);
  stepper.writeChopperMode      (TMC5130::SpreadCycleMode);                 //driver.writeChopperMode(SpreadCycleMode);
  stepper.setPosition           (0);                                       //controller.zeroActualPosition(); XACTUAL
  stepper.setTarget             (home_parameters.target_position);         //controller.writeTargetPosition();  XTARGET
  stepper.setMaxVelocity        (home_parameters.velocity);                //VMAX   controller.writeMaxVelocity(home_parameters.velocity);
  stepper.setSecondAcceleration (home_parameters.acceleration);            //AMAX  controller.writeMaxAcceleration(home_parameters.acceleration);
  stepper.setTZeroWait          (home_parameters.zero_wait_duration);      //TZEROWAIT  controller.writeZeroWaitDuration(home_parameters.zero_wait_duration);
  stepper.setRampMode           (TMC5130::PositionMode);                //RAMPMODE   controller.writeRampMode(PositionMode);
}

/*void InitGoto(TMC5130 &stepper, bool ResetPosition){
  stepper.setRampMode(TMC5130::PositionMode); //RAMPMODE: set position
  //Stop
  stepper.setMaxVelocity(0);  //To Stop
  if(ResetPosition) stepper.setPosition(0);
  stepper.setTarget(0);

  stepper.setStartVelocity      (   0);  //Set VSTART=0. Higher velcoity for abrupt start (limited by motor).
  stepper.setStopVelocity       (  10);  //Set VSTOP=10, but not below VSTART. Higher velocity for abrupt stop.
  stepper.setFirstAcceleration  (   0);  //Set acceleration A1 as desired by application
  stepper.setSecondDeceleration (  10);  //D1: Use same value as A1 or higher
  stepper.setFirstVelocity      (   0);  //Determine velocity, where max. motor torque or current sinks appreciably, write to V1

  stepper.setCurrent(15, 31, 7); //setCurrent(31, 31, 15);

}
*/