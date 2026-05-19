#include <Arduino.h>
#include "../SysSampler.h"
#include "Utilities.h"

#define USE_HID_SAMPLER
#define USE_STRIPLED
#define USE_EXPANDERS
#define USE_SPI
#define USE_TMC5130  //Require USE_EXPANDERS and USE_SPI
//#define USE_STEPPERS  //Require USE_EXPANDERS and USE_SPI



void ExecuteCommand(const uint8_t* data, uint16_t len);

#if !(defined(USE_HID_SAMPLER) | defined(USE_SERIALIN))
  #warning None communication system
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
    TMC5130(SPI,  6,  7, SpiEnableSteppers, SPI_FREQ, "Motor A: Up/Dn"),
    TMC5130(SPI,  4,  5, SpiEnableSteppers, SPI_FREQ, "Motor B: Left/Right"),
    TMC5130(SPI,  2,  3, SpiEnableSteppers, SPI_FREQ, "Motor C: Syringe/Diluter"),
    TMC5130(SPI, 12, 13, SpiEnableSteppers, SPI_FREQ, "Motor D: Depositor"),
    TMC5130(SPI, 10, 11, SpiEnableSteppers, SPI_FREQ, "Motor E"),
    TMC5130(SPI,  8,  9, SpiEnableSteppers, SPI_FREQ, "Motor F")
  };

void cSteppers_InitFreeRotate(uint8_t MotIdx, bool Direction, bool Free=true){
  Steppers[MotIdx].setStops(Direction ? TMC5130::ForwardDirection : TMC5130::ReverseDirection);
	Steppers[MotIdx].setCurrent(7, 1, 1);
  if(Free)  SetFreeRunning(Steppers[MotIdx], 2, 0, Direction);
}

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
    cSteppers_InitFreeRotate(0, true, false);    //cSteppers_InitFreeRotate(1, false, false);

    Steppers[0].InitGoTo(0, 10, 0, 10, 0);
    Steppers[0].setPosition(0);
    Steppers[0].setMicrosteps(8);
    Steppers[0].SetTrapezoidal(100, 3000); //setSecondAcceleration=setFirstDeceleration, setMaxVelocity
    Steppers[0].setTarget(20000); //XTARGET
    Steppers[0].setRampMode(TMC5130::PositionMode);
    Serial.println("Avvio Uno.\n");


    Steppers[1].InitGoTo(0, 10, 0, 10, 0);

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

void ExecuteCommand(const uint8_t* data, uint16_t len){
  bool AnswerSent = false;
  sCommand Cmd;
  memcpy(&Cmd, data, sizeof(sCommand));

  //ToDo, AAA I suppose Cmd.m_MsgType==eTypCommand
  Serial.println("Execution");
  switch(Cmd.m_SubSystem){
    case eExpanders:
      Serial.println("Espanders' Command");
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
      Serial.println("TMC5130's Command");
      {
        TmcAnswer Answer;
        Answer.m_Motor    = Cmd.m_Par[0];
        AnswerSent = true;
        switch(Cmd.m_Cmd){
          case  97: Serial.println("Chip Enable");    Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].SetChipEnable(Cmd.m_Par[1]!=0);                        break;
          case  98: Serial.println("Init Stops");     Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setStops((Cmd.m_Par[1]!=0)?TMC5130::ForwardDirection:TMC5130::ReverseDirection);            break;
          case  99: Serial.println("Set Currents");   Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setCurrent(Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3]);  break;
          case 100: Serial.println("Set Position");   Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setPosition  (Cmd.m_Par[1]);                           break;
          case 101: Serial.println("Set MicroStep");  Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setMicrosteps(Cmd.m_Par[1]);                           break;
          case 102: Serial.println("Set Target");     Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setTarget    (Cmd.m_Par[1]);                           break;
          case 103: Serial.println("SetTrapezoidal"); Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].SetTrapezoidal(Cmd.m_Par[1], Cmd.m_Par[2]);            break;
          case 104: Serial.println("Set Ramp Mode");  Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setRampMode((TMC5130::RampMode)Cmd.m_Par[1]);          break;
          case 105: Serial.println("Set Timer");      Answer.m_Result = (Steppers[Cmd.m_Par[0]].SetTimer(Cmd.m_Par[1]*1000) ?eCmdOk : eCmdRetry);             break;
          case 106: Serial.println("Wait Timer");     Answer.m_Result = (Steppers[Cmd.m_Par[0]].WaitTimer()                 ?eCmdOk : eCmdRetry);             break;
          case 107: Serial.println("Wait Stop");      Answer.m_Result = (Steppers[Cmd.m_Par[0]].FSA_WaitEndOfSteps  ()      ?eCmdOk : eCmdRetry);             break;
          case 108: Serial.println("Init GoTo");      Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].InitGoTo         (Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3], Cmd.m_Par[4], Cmd.m_Par[5]); break;
          case 109: Serial.println("FreeRunning");    Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].SetFreeRunning   (Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3]);                             break;
          case 110: Serial.println("Set Accel..s");   Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setAccelerations ( (TMC5130::eAccelerations)Cmd.m_Par[1], Cmd.m_Par[2]);                             break;
          case 111: Serial.println("Set Velocities"); Answer.m_Result = eCmdOk; Steppers[Cmd.m_Par[0]].setVelocities    ( (TMC5130::eVelocity)Cmd.m_Par[1], Cmd.m_Par[2]);                             break;

          default:  Serial.println("Unknown TMC5130's command"); AnswerSent = false; break;
        }
        Answer.m_Remaining  = Steppers[Cmd.m_Par[0]].getRemaining()/1000;
        Answer.m_spiStatus  = Steppers[Cmd.m_Par[0]].GetSpiStatus().bytes;
        Answer.m_Ioin8      = (Steppers[Cmd.m_Par[0]].getIoin().bytes & 0xFF);
        Answer.m_Velocity   = Steppers[Cmd.m_Par[0]].getVelocity();
        Answer.m_Position   = Steppers[Cmd.m_Par[0]].getPosition();
        Answer.m_xTarget    = Steppers[Cmd.m_Par[0]].getTarget();
        Answer.m_Currents   = Steppers[Cmd.m_Par[0]].getCurrents();

        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); //AnswerSent = true;
      }
    break;
#endif
#if defined(USE_STEPPERS)
    case eSteppers:
      {
          switch(Cmd.m_Cmd){
            case 102: // 'f' = FreeRotation
              Serial.printf("FreeRotation: Motor %d, Direction:%s\n", (int)Cmd.m_Par[0], (Cmd.m_Par[1]==0)?"Left":"Right");
              Motors.InitFreeRotate(Cmd.m_Par[0], (Cmd.m_Par[1]==0));
              break;
            case 104: // 'h' = Halt
              //if(Cmd.m_PatLen==2){
              Serial.printf("Halt: Motor=%d, Decel=%d\n", (int)Cmd.m_Par[0], (int)Cmd.m_Par[1]);
              Motors.Halt(Cmd.m_Par[0], Cmd.m_Par[1]);
              break;
            case 119: //'w': WaitEndCommand
              {
                bool Status = Motors.WaitEnd(Cmd.m_Par[0]);
                Serial.printf("Halt: Motor=%d, FSA Is %s\n", (int)Cmd.m_Par[0], Status?"Stopped":"Running");
              }
              break;
            case 71: //'G': GoTo
              {
                bool Status = Motors.GoTo(Cmd.m_Par[0],Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3]);
                Serial.printf("GoTo: Motor=%d, %ld, %ld, %ld --> %s\n", (int)Cmd.m_Par[0], Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3], Status?"True":"False");
              }
              break;
            case 67:  //'C' Set Currents
              {
                bool Status = Motors.setCurrents(Cmd.m_Par[0], Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3]);
                Serial.printf("setCurrents: Motor=%d, %ld, %ld, %ld --> %s\n", (int)Cmd.m_Par[0], Cmd.m_Par[1], Cmd.m_Par[2], Cmd.m_Par[3], Status?"True":"False");
              }
              break;
            default: Serial.println("Unknown Stepper command"); break;
          }
          StepperAnswer Answer;
          Answer.m_Cmd        = Cmd.m_Cmd;
          Answer.m_MasterId   = Cmd.m_MasterId;
          Answer.m_DetailProg = Cmd.m_DetailProg;
          Answer.m_Result     = eCmdOk;  //ToDo AAA
          Answer.m_Motor      = Cmd.m_Par[0];
          Answer.m_spiStatus  = 0xFF; //ToDo
          SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) ); AnswerSent = true;
      }
      break;
#endif
    case eStripLed:
      {
        bool Res = false;
        StripAnswer Answer;
        switch(Cmd.m_Cmd){
          case 108: Answer.m_Result = StripLed.SetGame((cStripLed::StripGame)Cmd.m_Par[0]) ? eCmdOk : eCmdRetry;  break; //Led Effect
          case 103: Answer.m_Result = eCmdOk;                                                                     break; //Ask Delay
          case 100: Answer.m_Result = StripLed.SetTimer( Cmd.m_Par[0]*1000 )  ? eCmdOk : eCmdRetry;               break; //Delay
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
    default:
      Serial.printf("Unknown System: %d ('%c')\n", (int)Cmd.m_SubSystem, Cmd.m_SubSystem);
      break;
  }
  Serial.println("Answer");
  if(!AnswerSent){
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
