#include <Arduino.h>
#include "../SysSampler.h"
#include "Utilities.h"

//#define USE_ESPNOW        //486689 bytes
//#define USE_STRIPLED    //365675 bytes
#define USE_HID_SAMPLER
#define USE_EXPANDERS
#define USE_SPI
#define USE_TMC5130     //Require USE_EXPANDERS and USE_SPI
//#define USE_STEPPERS  //Require USE_EXPANDERS and USE_SPI

#define PRINTLOG(s) Serial.print(F(s));


void ExecuteCommand(const uint8_t* data, uint16_t len);

#if !(defined(USE_HID_SAMPLER) | defined(USE_SERIALIN))
  #warning None communication system
#endif

#if defined(USE_ESPNOW)
  #include "src/ESPNOW/SamplerNow.h"
#endif

#if defined(USE_HID_SAMPLER)
  #include "HID_Sampler.h"

  SamplerHIDDevice SamplerHID;
#endif

#if defined(USE_EXPANDERS)
  #include "src/EXPANDERS/Expanders.h"

  TCA9555   Expanders[] = { TCA9555(0x20), TCA9555(0x21), TCA9555(0x22), };
  cExpSampler ExpSampler;
#endif

#if defined(USE_SPI)
  #include <SPI.h>
  #define SPI_SCK   41
  #define SPI_MISO  42
  #define SPI_MOSI  40
  #define SPI_FREQ  4000000
#endif

#if defined(USE_TMC5130)
  #if !defined(USE_EXPANDERS) || !defined(USE_SPI)
    #error Steppers require Expanders and SPI
  #endif
  #include "src/STEPPERS/TMC5130.h"
  #include "src/STEPPERS/TMC5130_Inits.h"
  #define ExpanderStepper 1
  void  SpiEnableSteppers(uint8_t csPin, bool en) { Expanders[ExpanderStepper].write1(csPin, en?0:1); }  //Callback for Chip-Select through expander
  
  TMC5130 Steppers[]={
/*
#define X(eMotorId, csPin, cePin, description) TMC5130(SPI,  csPin,  cePin, SpiEnableSteppers, SPI_FREQ, description),
	STEPPERS_LIST
#undef X
*/
    TMC5130(SPI,  6,  7, SpiEnableSteppers, SPI_FREQ, "Motor A: Up/Dn"),
    TMC5130(SPI,  4,  5, SpiEnableSteppers, SPI_FREQ, "Motor B: Left/Right"),
    TMC5130(SPI,  2,  3, SpiEnableSteppers, SPI_FREQ, "Motor C: Syringe/Diluter"),
    TMC5130(SPI, 12, 13, SpiEnableSteppers, SPI_FREQ, "Motor D: Depositor"),
    TMC5130(SPI, 10, 11, SpiEnableSteppers, SPI_FREQ, "Motor E"),
    TMC5130(SPI,  8,  9, SpiEnableSteppers, SPI_FREQ, "Motor F")
  };

#endif

#if defined(USE_STEPPERS)
  #if !defined(USE_EXPANDERS) || !defined(USE_SPI)
    #error Steppers require Expanders and SPI
  #endif
  #include "src/STEPPERS/cSteppers.h"
  cSteppers Motors;
#endif

#if defined(USE_STRIPLED)
  #include "src/STRIPLED/StripLed.h"
  cStripLed StripLed;
#endif

void setup() {
  Serial.begin(115200); delay(1000);

  #if defined(USE_HID_SAMPLER)
    SamplerHID.Setup();
    SamplerHID.EnableLog(false);
    SamplerHID.SetParser(ExecuteCommand);
  #endif

  #if defined(USE_STRIPLED)
    StripLed.Init();
  #endif

  #if defined(USE_EXPANDERS)
    ExpSampler.Exp_Setup();
  #endif

  #if defined(USE_SPI)
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  #endif

  #if defined(USE_STEPPERS)
    Motors.TestAllSteppers();
    Motors.Setup();
  #endif

  #if defined(USE_TMC5130)
  	for(int i=0; i<wxSIZEOF(Steppers); i++){
      Expanders[ExpanderStepper].write1(Steppers[i].getcePinAddress(), 0);  //0 Activate
      TMC5130_Init_00(Steppers[i]);
    }
    //Test All Steppers
  	for(int i=0; i<wxSIZEOF(Steppers); i++){
      bool IsChipEnabled = Expanders[ExpanderStepper].read1(Steppers[i].getcePinAddress())==0;  //0 Activate
	    bool test = Steppers[i].IsConnected();
	    Serial.printf("\t%-25s %3s, Chip is %8s\n", Steppers[i].GetName(), test?"Ok":"KO", IsChipEnabled?"Enabled":"Disabled");
	    if(test==false){
    	  Serial.println("Not all steppers are OK.\n");
	      while (1);
      }
    }

  	for(int i=0; i<wxSIZEOF(Steppers); i++){
      Steppers[i].setMicrosteps(8);
      Steppers[i].setMotorDirection (TMC5130::ForwardDirection);
      Steppers[i].DisableStops();
    }

    /*
    //Steppers[0].InitGoTo(0, 10, 0, 10, 0);
      setStartVelocity      (0);	//Set VSTART=0. Higher velocity for abrupt start (limited by motor).
      setStopVelocity       (10);	//Set VSTOP=10, but not below VSTART. Higher velocity for abrupt stop.
      setFirstAcceleration  (0);	//A1 Set acceleration A1 as desired by application
      setSecondDeceleration (10);	//D1: Use same value as A1 or higher
      setFirstVelocity      (0);	//V1: Determine velocity, where max. motor torque or current sinks appreciably, write to V1

    Steppers[0].setPosition(0);
    Steppers[0].setMicrosteps(8);

    //Steppers[0].SetTrapezoidal(100, 3000); //setSecondAcceleration=setFirstDeceleration, setMaxVelocity
      setSecondAcceleration  (100);  //AMAX  [μsteps / ta²]  0...1048575=0xFFFFF Second acceleration between V1 and VMAX (unsigned)
      setFirstDeceleration   (100);  //DMAX  [μsteps / ta²]  0...1048575=0xFFFFF Deceleration between VMAX and V1 (unsigned)
      setMaxVelocity         (3000);  //VMAX  0...8388096=7FFE00
      writeReg(TZEROWAIT, 0);


    Steppers[0].setTargetBase(20000); //XTARGET
    Steppers[0].setRampMode(TMC5130::PositionMode);

    //Steppers[1].InitGoTo(0, 10, 0, 10, 0);
  */
    Serial.println("End Setup Motors.\n");
  #endif

  #if defined(USE_ESPNOW)
    sNowFsaRx_Setup();
  #endif

}

void AlwaysRun(void){
  #if defined(USE_STRIPLED)
    StripLed.AlwaysRun();
  #endif
  #if defined(USE_STEPPERS)
    Motors.Loop();
  #endif
  yield();
}


#if defined(USE_TMC5130)
  void FillAnswer(TmcAnswer& Answer, uint8_t Motor){
    if(Motor>wxSIZEOF(Steppers)) return;
        Answer.m_Remaining  = Steppers[Motor].getRemaining()/1000;
        Answer.m_spiStatus  = Steppers[Motor].GetSpiStatus().bytes;
        Answer.m_Ioin8      =(Steppers[Motor].getIoin().bytes & 0xFF);  //IOIN
        Answer.m_Position   = Steppers[Motor].getPosition();             //XACTUAL
        Answer.m_xTarget    = Steppers[Motor].getTarget();               //XTARGET
        Answer.m_Currents   = Steppers[Motor].getCurrents();             //ShadowRegs.Ihold_Irun
#if defined(X_SHOW_CHOPCONF)
        Answer.m_CHOPCONF   = Steppers[Motor].getChopconf().bytes;       //CHOPCONF
#endif
        Answer.m_DRV_STATUS = Steppers[Motor].getDrvStatus().bytes;      //DRV_STATUS
        Answer.m_MSCURACT   = Steppers[Motor].getMscuract().bytes;       //MSCURACT

        Answer.m_VSTART     = Steppers[Motor].getStartVelocity(); //18 bits limited to 16
        Answer.m_V1         = Steppers[Motor].getFirstVelocity();	//20 bits limited to 16
        Answer.m_VMAX       = Steppers[Motor].getMaxVelocity();		//23 bits limited to 16
        Answer.m_VSTOP      = Steppers[Motor].getStopVelocity();	//18 bits limited to 16
        Answer.m_VACTUAL    = Steppers[Motor].getVelocity();      //23 bits limited to 16

        Answer.m_A1         = Steppers[Motor].getFirstAcceleration();		//16 bits
        Answer.m_AMAX       = Steppers[Motor].getSecondAcceleration();	//16 bits
        Answer.m_DMAX       = Steppers[Motor].getFirstDeceleration();		//16 bits
        Answer.m_D1         = Steppers[Motor].getSecondDeceleration();	//16 bits
#if defined(SHOW_GCONF)
        Answer.m_GCONF      = Steppers[Motor].getGconf().bytes;         //16 bits
#endif
#if defined(SHOW_SWMODE)
        Answer.m_SWMODE     = Steppers[Motor].getSwMode().bytes;        //16 bits
#endif
  }
#endif


void ExecuteCommand(const uint8_t* data, uint16_t len){
  bool AnswerSent = false;
  sCommand Cmd;
  memcpy(&Cmd, data, sizeof(sCommand));

  //ToDo, AAA I suppose Cmd.m_MsgType==eTypCommand
  Serial.printf("Execution (Step %d):", Cmd.m_DetailProg);
  switch(Cmd.m_SubSystem){
    case eSystemCmd:
      Serial.println("System Command");
      switch(Cmd.m_Cmd){
        case 48:
          {
            sAnswerVersion Answer;
            Answer.Y = getYear2 (__DATE__);
            Answer.M = getMonth (__DATE__);
            Answer.D = getDay   (__DATE__);
            Answer.h = getHour  (__TIME__);
            Answer.m = getMinute(__TIME__);
            Answer.s = getSecond(__TIME__);            
            SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); AnswerSent = true;            
          }
          break;

        default: Serial.printf("Unknown Sistem command (%d=%02x)\n", (int)Cmd.m_Cmd, (int)Cmd.m_Cmd); break;
      }
      break;

    case eExpanders:
      Serial.println(F("Espanders' Command"));
      switch(Cmd.m_Cmd){
        case 118: //'v'
          {
            uint16_t Mask = ExpSampler.getMask((cExpSampler::eExpOutputs)Cmd.m_Par[0]);
            bool Enable = (Cmd.m_Par[1]!=0);
            ExpSampler.WriteOut(Mask, Enable);
            Serial.printf("Set %d pattern %04X to '%s'\n", (int)Cmd.m_Par[0], Mask, Enable?"True":"False");
          }
          break;
        default:
          Serial.printf("Unknown Expanders' command (%d=%02x)\n", (int)Cmd.m_Cmd, (int)Cmd.m_Cmd);
          break;
      }
      {
        sExpanderStandard Answer;
        Answer.m_CurrStatus = ExpSampler.GetLastOutput();
        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); AnswerSent = true;
      }
      break;
#if defined(USE_TMC5130)
    case eStepDirect:
    case eStepNoMotor:
      Serial.printf("\"TMC5130's Command %s\", \"", (Cmd.m_SubSystem==eStepNoMotor)?"No Motor":"");
      {
        static uint8_t  CM = 0;
        uint8_t         pr = 0; //Param Index
        uint8_t         CurrentMotor  = (Cmd.m_SubSystem==eStepNoMotor) ? CM : Cmd.m_Par[pr++];
        uint8_t         ParNum        = (Cmd.m_SubSystem==eStepNoMotor) ? Cmd.m_PatLen : Cmd.m_PatLen-1;
        TmcAnswer       Answer;
        AnswerSent = true;
        switch(Cmd.m_Cmd){
          case '0': Serial.print(F("Do Nothing"));     Answer.m_Result = eCmdOk;                                                                               break;
          case '1': Serial.print(F("Change Motor"));   Answer.m_Result = eCmdOk; CM = Cmd.m_Par[pr++];
          #if defined(USE_STRIPLED)
            StripLed.setNumShowed(CM);
          #endif
              break;
          case '2': Serial.print(F("Set Register"));   Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetReg(Cmd.m_Par[pr++], Cmd.m_Par[1]);                 break;
          case 'a': Serial.print(F("Chip Enable"));    Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetChipEnable(Cmd.m_Par[pr++]!=0);
                                                                              Steppers[CurrentMotor].TestReset();
                                                                              Steppers[CurrentMotor].getGstat();                                            break;
          case 'b': Serial.print(F("Set EndStops"));      Answer.m_Result = eCmdOk;
                    switch(ParNum){
                      case 0: Steppers[CurrentMotor].DisableStops();                                        break;
                      case 1: if(Cmd.m_Par[pr]>1) Steppers[CurrentMotor].DisableStops();
                              else                Steppers[CurrentMotor].setStops(Cmd.m_Par[pr++]!=0);      break;
                      case 7:                     Steppers[CurrentMotor].setStops(Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0);  break;
                    }
                    break;
                    //if(ParNum==0 || Cmd.m_Par[pr]>1)  Steppers[CurrentMotor].DisableStops();
                    //else                              Steppers[CurrentMotor].setStops(Cmd.m_Par[pr++]!=0);
                    //                                                                                                                                        break;
          case 'c': PRINTLOG("Set Currents");   Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setCurrent   (Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++]);  break;
          case 'd': PRINTLOG("Set Position");   Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setPosition  (Cmd.m_Par[pr++]);                           break;
          case 'e': PRINTLOG("Set MicroStep");  Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setMicrosteps(Cmd.m_Par[pr++]);                           break;
          case 'f': PRINTLOG("Set Target");     Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setTargetBase(Cmd.m_Par[pr++]);                           break;

          case 'g': switch(ParNum){
                      case 3:   PRINTLOG("Set Ramp Trapezoidal");       Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetRamp(Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], 0);  break;
                      case 5:   PRINTLOG("Set Ramp Six Points Simple"); Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetRamp(Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], 0);  break;
                      case 8:   PRINTLOG("Set Ramp Six Points");        Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetRamp(Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], 0);  break;
                      case 2:
                      default:  PRINTLOG("Set Trapezoidal");            Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetTrapezoidal(Cmd.m_Par[pr++], Cmd.m_Par[pr++]);            break;                      
                    }
                    break;
          case 'h': PRINTLOG("Set Ramp Mode");  Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setRampMode((TMC5130::RampMode)Cmd.m_Par[pr++]);                                         break;

          case 'i': PRINTLOG("Set Timer");      Answer.m_Result = (Steppers[CurrentMotor].SetTimer(Cmd.m_Par[pr++]*1000) ?eCmdOk : eCmdRetry);                                            break;
          case 'j': PRINTLOG("Generic Wait");   Answer.m_Result = (Steppers[CurrentMotor].WaitMotor((TMC5130::eWaitingMotor)Cmd.m_Par[pr++], Cmd.m_Par[pr++])  ?eCmdOk : eCmdRetry);
                    Serial.printf(" Result is %d ", (int)Answer.m_Result);
             break;

          case 'k': PRINTLOG("Advance");        Answer.m_Result = eCmdOk; Steppers[CurrentMotor].Advance(Cmd.m_Par[pr++]);                           break;


          case 'l': PRINTLOG("Init GoTo");      Answer.m_Result = eCmdOk; Steppers[CurrentMotor].InitGoTo         (Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++]); break;
          case 'm': PRINTLOG("FreeRunning");    Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetFreeRunning   (Cmd.m_Par[pr++], Cmd.m_Par[pr++], Cmd.m_Par[pr++]);                          break;
          case 'n': PRINTLOG("Set Accel..s");   Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setAccelerations ( (TMC5130::eAccelerations)Cmd.m_Par[pr++], Cmd.m_Par[pr++]);                 break;
          case 'o': PRINTLOG("Set Velocities"); Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setVelocities    ( (TMC5130::eVelocity)Cmd.m_Par[pr++], Cmd.m_Par[pr++]);                      break;
          case 'p': PRINTLOG("Set Direction");  Answer.m_Result = eCmdOk; Steppers[CurrentMotor].setMotorDirection(Cmd.m_Par[pr++]==0 ? TMC5130::ForwardDirection : TMC5130::ReverseDirection); break;
          case 'q':
              {
                TMC5130::Gconf gconf = Steppers[CurrentMotor].getGconf();
                  switch(ParNum){
                    //"I_scale_analog", "internal_Rsense", "en_pwm_mode", "enc_commutation", "shaft"
                    case 5:       if(Cmd.m_Par[pr]) gconf.recalibrate_i_scale_analog     = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.faststandstill_internal_rsense = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.en_pwm_mode                    = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.multistep_filt_enc_commutation = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.shaft                          = Cmd.m_Par[pr]-1;
                      break;
                    //"diag0_error", "diag0_otpw", "diag0_stall", "diag1_stall", "diag1_index", "diag1_onstate", "diag1_steps_skipped"
                    case 7:       if(Cmd.m_Par[pr]) gconf.diag0_error              = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag0_otpw               = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag0_stall_int_step     = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag1_stall_poscomp_dir  = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag1_index              = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag1_onstate            = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.diag1_steps_skipped      = Cmd.m_Par[pr]-1;
                            //pr++;  diag0_int_pushpull
                            //pr++;  diag1_poscomp_pushpull
                      break;
                    //"small_hysteresis", "stop_enable", "direct_mode", "test_mode"})},
                    case 4:       if(Cmd.m_Par[pr]) gconf.small_hysteresis = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.stop_enable      = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.direct_mode      = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.test_mode        = Cmd.m_Par[pr]-1;
                      break;
                  }
                  Steppers[CurrentMotor].setGconf(gconf.bytes);
              }
              break;
          default:  PRINTLOG("Unknown TMC5130's NoMotor command"); AnswerSent = false; break;
        }
        PRINTLOG("\" ... ");
        Answer.m_Motor    = CurrentMotor;
        FillAnswer(Answer, CurrentMotor);
        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); //AnswerSent = true;
      }
      break;
#endif
#if defined(USE_STRIPLED)
    case eStripLed:
      {
        bool Res = false;
        StripAnswer Answer;
        switch(Cmd.m_Cmd){
          case 108: Answer.m_Result = StripLed.SetGame((StripGame)Cmd.m_Par[0]) ? eCmdOk : eCmdRetry;  break; //Led Effect
          case 103: Answer.m_Result = eCmdOk;                                                                     break; //Ask Delay
          case 100: Answer.m_Result = StripLed.SetTimer( Cmd.m_Par[0]*1000 )  ? eCmdOk : eCmdRetry;               break; //Delay
          case 110: Answer.m_Result = eCmdOk;  StripLed.setNumShowed(Cmd.m_Par[0]);                               break;
          case 114: Answer.m_Result = StripLed.ResetTimer()                   ? eCmdOk : eCmdRetry;               break; //ResetTimer
          case 119: Answer.m_Result = StripLed.WaitTimer()                    ? eCmdOk : eCmdRetry;               break; //WaitTimer
          default:  Answer.m_Result = eCmdError;                                                                  break; //
        }
        Answer.m_MsgType	  = eTypAnswStripLed;
        Answer.m_CurrGame   = StripLed.getCurrGame();
        Answer.m_Remaining  = StripLed.Remaining()/1000;
        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); AnswerSent = true;
      }
      break;
#endif
    default:
      Serial.printf("Unknown System: %d ('%c')\n", (int)Cmd.m_SubSystem, Cmd.m_SubSystem);
      break;
  }
  if(!AnswerSent){
      PRINTLOG("Generic Answer\n");
      ShowBuffer(data, len);
      Serial.printf("MsgType...: 0x%02X = %d = '%c'\n", (int)Cmd.m_MsgType,   (int)Cmd.m_MsgType,   (char)Cmd.m_MsgType);
      Serial.printf("SubSystem.: 0x%02X = %d = '%c'\n", (int)Cmd.m_SubSystem, (int)Cmd.m_SubSystem, (char)Cmd.m_SubSystem);
      Serial.printf("Cmd.......: 0x%02X = %d = '%c'\n", (int)Cmd.m_Cmd,       (int)Cmd.m_Cmd,       (char)Cmd.m_Cmd);
      for(int i=0;i<Cmd.m_PatLen; i++){
        Serial.printf("Par.%2d....: %ld\n", i, Cmd.m_Par[i]);
      }

      sAnswerStandard Answer;
      Answer.m_SubSystem  = Cmd.m_SubSystem;
      Answer.m_Cmd        = Cmd.m_Cmd;
      Answer.m_UnknownMsg = Cmd.m_MsgType;
      SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
  }
}  

void loop() {
  AlwaysRun();
}
