#pragma once
#include "HID_Sampler.h"


#if defined(USE_TMC5130)
  void FillAnswer(TmcAnswer& Answer, uint8_t Motor){
    if(Motor>wxSIZEOF(Steppers)) return;
        Answer.m_Remaining  = Steppers[Motor].getRemaining()/1000;
        Answer.m_spiStatus  = Steppers[Motor].GetSpiStatus().bytes;
        Answer.m_Ioin8      =(Steppers[Motor].getIoin().bytes & 0xFF);  //IOIN
        Answer.m_Position   = Steppers[Motor].getPosition();            //XACTUAL
        Answer.m_xTarget    = Steppers[Motor].getTarget();              //XTARGET
        Answer.m_Currents   = Steppers[Motor].getCurrents();            //ShadowRegs.Ihold_Irun
#if defined(X_SHOW_CHOPCONF)
        Answer.m_CHOPCONF   = Steppers[Motor].getChopconf().bytes;      //CHOPCONF
#endif
        Answer.m_DRV_STATUS = Steppers[Motor].getDrvStatus().bytes;     //DRV_STATUS
        Answer.m_MSCURACT   = Steppers[Motor].getMscuract().bytes;      //MSCURACT

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

#if defined(USE_INA260)
        Answer.m_Curr   = sina260.getVal(cINA260::eCurr);
        Answer.m_Volt   = sina260.getVal(cINA260::eVolt);
        Answer.m_Power  = sina260.getVal(cINA260::ePwr);
#endif

  }
#endif



bool Exec_SystemCmd(SamplerHIDDevice& SamplerHID, sCommand& Cmd){ //cINA260   sina260
  Serial.println("System Command");
  switch(Cmd.m_Cmd){
    case 48:  //'0'
      {
        sAnswerVersion Answer;
        Answer.m_Cmd    = Cmd.m_Cmd;
        Answer.m_Result = eCmdOk;

        Answer.Y = getYear2 (__DATE__);
        Answer.M = getMonth (__DATE__);
        Answer.D = getDay   (__DATE__);
        Answer.h = getHour  (__TIME__);
        Answer.m = getMinute(__TIME__);
        Answer.s = getSecond(__TIME__);            
        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
      }
      return true;
#if defined(USE_INA260)
    case 49:  //'1'
      {
        sAnswerPower Answer;
        Answer.m_Cmd    = Cmd.m_Cmd;
        Answer.m_Result = eCmdOk;
        Answer.Curr     = sina260.getVal(cINA260::eCurr);
        Answer.Volt     = sina260.getVal(cINA260::eVolt);
        Answer.Power    = sina260.getVal(cINA260::ePwr);
        SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
      }
      return true;
#endif
    default: Serial.printf("Unknown Sistem Command (%d=%02x)\n", (int)Cmd.m_Cmd, (int)Cmd.m_Cmd); break;
  }
  return false;
}

#if defined(USE_EXPANDERS)
  bool Exec_ExpandersCmd(SamplerHIDDevice& SamplerHID, sCommand& Cmd){ //cExpSampler ExpSampler, TCA9555   Expanders[]
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
            return false;
        }
        {
          sExpanderStandard Answer;
          Answer.m_Cmd        = Cmd.m_Cmd;
          Answer.m_Result     = eCmdOk;
          Answer.m_CurrStatus = ExpSampler.GetLastOutput();
          SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
        }
        return true;
  }
#endif

#if defined(USE_STRIPLED)
  bool Exec_StripLed(SamplerHIDDevice& SamplerHID, sCommand& Cmd){ //cStripLed StripLed
    StripAnswer Answer;
    switch(Cmd.m_Cmd){
      case 108: Answer.m_Result = StripLed.SetGame((StripGame)Cmd.m_Par[0]) ? eCmdOk : eCmdRetry;             break; //Led Effect
      case 103: Answer.m_Result = eCmdOk;                                                                     break; //Ask Delay
      case 100: Answer.m_Result = StripLed.SetTimer( Cmd.m_Par[0]*1000 )  ? eCmdOk : eCmdRetry;               break; //Delay
      case 110: Answer.m_Result = eCmdOk;  StripLed.setNumShowed(Cmd.m_Par[0]);                               break;
      case 114: Answer.m_Result = StripLed.ResetTimer()                   ? eCmdOk : eCmdRetry;               break; //ResetTimer
      case 119: Answer.m_Result = StripLed.WaitTimer()                    ? eCmdOk : eCmdRetry;               break; //WaitTimer
      default:  Answer.m_Result = eCmdError;
      return false;
    }
    Answer.m_MsgType	  = eTypAnswStripLed;
    Answer.m_CurrGame   = StripLed.getCurrGame();
    Answer.m_Remaining  = StripLed.Remaining()/1000;
    Answer.m_Cmd        = Cmd.m_Cmd;
    SamplerHID.SendBuffer((uint8_t*)&Answer, sizeof(Answer) );
    return true;
  }
#endif

#if defined(USE_TMC5130_FSA)
  bool Exec_SteppersFSA(SamplerHIDDevice& SamplerHID, sCommand& Cmd){ //TMC5130_FSA Steppers[]
    Serial.printf("\"FSA single TMC5130\"");
    {
      FsaSingleAnswer Answer;
      Answer.m_Cmd = Cmd.m_Cmd; // ToDo: use proper Answer
      uint8_t         pr = 0; //Param Index
      uint8_t         CurrentMotor  = Cmd.m_Par[pr++];
      switch(Cmd.m_Cmd){
        case '0': PRINTLOG("Wait Command"); Answer.m_Result = Steppers[CurrentMotor].Exec_WaitOperations()      ? eCmdOk : eCmdRetry; break;
        case 'a': PRINTLOG("Init Motor");   Answer.m_Result = Steppers[CurrentMotor].Exec_SearchBegin()         ? eCmdOk : eCmdRetry;   break;
        case 'b': 
          switch(Cmd.m_PatLen){
            case 2: PRINTLOG("GoTo2");         Answer.m_Result = Steppers[CurrentMotor].Exec_GoTo(Cmd.m_Par[pr++]                         ) ? eCmdOk : eCmdRetry; break;
            case 3: PRINTLOG("GoTo3");         Answer.m_Result = Steppers[CurrentMotor].Exec_GoTo(Cmd.m_Par[pr++],  Cmd.m_Par[pr++]*1000  ) ? eCmdOk : eCmdRetry; break;
          }
          break;
        default:  PRINTLOG("Unknown FSA single TMC5130 command"); 
        return false;                                                 break;
      }
      Answer.m_Motor      = CurrentMotor;
      Answer.m_FsaStatus  = Steppers[CurrentMotor].AskStatus();
      Answer.m_VACTUAL    = Steppers[CurrentMotor].getVelocity();      //23 bits limited to 16
      Answer.m_Position   = Steppers[CurrentMotor].getPosition();      //XACTUAL
      Answer.m_xTarget    = Steppers[CurrentMotor].getTarget();        //XTARGET
      Answer.m_Currents   = Steppers[CurrentMotor].getCurrents();      //ShadowRegs.Ihold_Irun
#if defined(USE_INA260)
        Answer.m_Curr   = sina260.getVal(cINA260::eCurr);
        Answer.m_Volt   = sina260.getVal(cINA260::eVolt);
        Answer.m_Power  = sina260.getVal(cINA260::ePwr);
#endif

      SamplerHID.SendBuffer( (uint8_t*)&Answer, sizeof(Answer) );
    }
    return true;
  }
#endif

bool Exec_Steppers(SamplerHIDDevice& SamplerHID, sCommand& Cmd){ //Steppers[]
  Serial.printf("\"TMC5130's Command %s\", \"", (Cmd.m_SubSystem==eStepNoMotor)?"No Motor":"");
      Serial.printf("\"TMC5130's Command %s\", \"", (Cmd.m_SubSystem==eStepNoMotor)?"No Motor":"");
      {
        static uint8_t  CM = 0; //Motor to use
        uint8_t         pr = 0; //Param Index
        uint8_t         CurrentMotor  = (Cmd.m_SubSystem==eStepNoMotor) ? CM : Cmd.m_Par[pr++];
        uint8_t         ParNum        = (Cmd.m_SubSystem==eStepNoMotor) ? Cmd.m_PatLen : Cmd.m_PatLen-1;
        TmcAnswer       Answer;
        Answer.m_Cmd = Cmd.m_Cmd;
        switch(Cmd.m_Cmd){
          case '0': PRINTLOG("Do Nothing");    Answer.m_Result = eCmdOk;                                                                               break;
          case '1': PRINTLOG("Change Motor");  Answer.m_Result = eCmdOk; CM = Cmd.m_Par[pr++];
          #if defined(USE_STRIPLED)
            StripLed.setNumShowed(CM);
          #endif
              break;
          case '2': PRINTLOG("Set Register");  Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetReg(Cmd.m_Par[pr++], Cmd.m_Par[1]);                 break;
          case 'a': PRINTLOG("Chip Enable");   Answer.m_Result = eCmdOk; Steppers[CurrentMotor].SetChipEnable(Cmd.m_Par[pr++]!=0);
                                                                         Steppers[CurrentMotor].ClearError();
                                                                         Steppers[CurrentMotor].getGstat();                                            break;
          case 'b': PRINTLOG("Set EndStops");  Answer.m_Result = eCmdOk;
                    switch(ParNum){
                      case 0: Steppers[CurrentMotor].DisableStops();                                        break;
                      case 1: if(Cmd.m_Par[pr]>1) Steppers[CurrentMotor].DisableStops();
                              else                Steppers[CurrentMotor].setStops(Cmd.m_Par[pr++]!=0);      break;
                      case 7:                     Steppers[CurrentMotor].setStops(Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0, Cmd.m_Par[pr++]!=0);  break;
                    }
                    break;
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
          case 'j': switch(ParNum){
                      case 2: PRINTLOG("Generic Wait");
                              Answer.m_Result = (Steppers[CurrentMotor].WaitMotor((TMC5130::eWaitingMotor)Cmd.m_Par[pr++], Cmd.m_Par[pr++])  ?eCmdOk : eCmdRetry);
                              Serial.printf(" Result is %d ", (int)Answer.m_Result);
                              break;
                      case 3: PRINTLOG("Wait Position");
                              Answer.m_Result = (Steppers[CurrentMotor].WaitPosition((TMC5130::eComparePosition)Cmd.m_Par[pr++], Cmd.m_Par[pr++], (bool)Cmd.m_Par[pr++]) ?eCmdOk : eCmdRetry );
                              break;
                    }
                    //AnswerSent = false; //For Debug
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
                    case 4:       if(Cmd.m_Par[pr]) gconf.small_hysteresis = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.stop_enable      = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.direct_mode      = Cmd.m_Par[pr]-1;
                            pr++; if(Cmd.m_Par[pr]) gconf.test_mode        = Cmd.m_Par[pr]-1;
                      break;
                  }
                  Answer.m_Result = eCmdOk;
                  Steppers[CurrentMotor].setGconf(gconf.bytes);
              }
              break;
          case 'z': PRINTLOG("Routine in Test");
                  #if defined(USE_TMC_Multi_FSA)
                    Answer.m_Result = MultiFSA.Exec_ResetAll() ? eCmdOk : eCmdRetry;
                  #else
                    Answer.m_Result = eCmdOk;
                    Steppers[CurrentMotor].Exec_SearchBegin();
                  #endif
              break;
            break;
          default:  PRINTLOG("Unknown TMC5130's NoMotor command"); 
            return false;
        }
        PRINTLOG("\" ... ");
        Answer.m_Motor    = CurrentMotor;
        FillAnswer(Answer, CurrentMotor);
        //For debug if Timer, show answer
        if( (Cmd.m_SubSystem==eStepDirect || Cmd.m_SubSystem==eStepNoMotor)
          && Cmd.m_Cmd=='j'
        ){
          PRINTLOG("Answer ... ");
          ShowBuffer( (uint8_t*)&Answer, sizeof(Answer) );
        }
        SamplerHID.SendBuffer( (uint8_t*)&Answer, sizeof(Answer) ); //AnswerSent = true;
      }

  return true;
}
