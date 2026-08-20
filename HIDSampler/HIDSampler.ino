#include <Arduino.h>
#include "../SysSampler.h"
#include "Utilities.h"



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

#if defined(USE_TMC5130) || defined(USE_TMC5130_FSA)
  #if !defined(USE_EXPANDERS) || !defined(USE_SPI)
    #error Steppers require Expanders and SPI
  #endif
  #include "src/STEPPERS/TMC5130.h"
  #include "src/STEPPERS/TMC5130_Inits.h"
  #include "src/STEPPERS/TMC5130_FSA.h"
  #define ExpanderStepper 1
  void  SpiEnableSteppers(uint8_t csPin, bool en) { Expanders[ExpanderStepper].write1(csPin, en?0:1); }  //Callback for Chip-Select through expander
  
  TMC5130_FSA Steppers[]={

#define X(eMotorId, csPin, cePin, description) TMC5130_FSA(SPI,  csPin,  cePin, SpiEnableSteppers, SPI_FREQ, description),
	STEPPERS_LIST
#undef X
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

#if defined(USE_TMC_Multi_FSA)
  #include "src/STEPPERS/TMC_Multi_FSA.h"

  TMC_Multi_FSA MultiFSA(Steppers, wxSIZEOF(Steppers));
#endif

#if defined(USE_INA260)
  #include "src/INA260/Ina260.h" //include also "Adafruit INA260 Library"
  cINA260   sina260;
#endif

#include "HID_Parsers.h"

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

  #if defined(USE_INA260)
    sina260.init();
  #endif

  #if defined(USE_STEPPERS)
    Motors.TestAllSteppers();
    Motors.Setup();
  #endif

  #if defined(USE_TMC5130) || defined(USE_TMC5130_FSA)
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

    Steppers[0].setOverSteps(12);      Steppers[0].setMicrosteps(8);  Steppers[0].ResetSpeed(7);
    Steppers[1].setOverSteps(10);      Steppers[1].setMicrosteps(8);  Steppers[1].ResetSpeed(20);
    Steppers[2].setOverSteps(100);     Steppers[2].setMicrosteps(4);  Steppers[2].ResetSpeed(2);  Steppers[2].IsRotative(true);


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

  #if defined(USE_INA260)
    sina260.AlwaysRun();
  #endif

  #if defined(USE_STEPPERS)
    Motors.Loop();
  #endif

  #if defined(USE_TMC5130) || defined(USE_TMC5130_FSA)
  	for(int i=0; i<wxSIZEOF(Steppers); i++){
      Steppers[i].FSA_SetHome_loop();
    }
  #endif

  #if defined(USE_TMC_Multi_FSA)
    MultiFSA.Multi_FSA_loop();
  #endif

  yield();
}






void ExecuteCommand(const uint8_t* data, uint16_t len){
  bool AnswerSent = false;
  sCommand Cmd;
  memcpy(&Cmd, data, sizeof(sCommand));

  Serial.printf("Execution (Step %d):", Cmd.m_DetailProg);
  switch(Cmd.m_SubSystem){
    case eSystemCmd:    AnswerSent = Exec_SystemCmd   (SamplerHID, Cmd);  break;

    #if defined(USE_EXPANDERS)
        case eExpanders:    AnswerSent = Exec_ExpandersCmd(SamplerHID, Cmd);  break;
    #endif

    #if defined(USE_STRIPLED)
        case eStripLed:     AnswerSent = Exec_StripLed    (SamplerHID, Cmd);  break;
    #endif

    #if defined(USE_TMC5130_FSA)
        case eSteppersFSA:  AnswerSent = Exec_SteppersFSA (SamplerHID, Cmd);  break;
    #endif

    #if defined(USE_TMC5130)
        case eStepDirect:
        case eStepNoMotor:  AnswerSent = Exec_Steppers    (SamplerHID, Cmd);  break;
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
      Answer.m_Result     = eCmdOk;
      Answer.m_UnknownMsg = Cmd.m_MsgType;
      SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
  }
}  

void loop() {
  AlwaysRun();
}
