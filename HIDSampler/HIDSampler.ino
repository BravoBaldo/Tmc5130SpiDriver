#include <Arduino.h>
#include "../SysSampler.h"

#define USE_HID_SAMPLER


#if !(defined(USE_HID_SAMPLER) | defined(USE_SERIALIN))
  #warning None communication system
#endif

#if defined(USE_HID_SAMPLER)
  #include "HID_Sampler.h"

  SamplerHIDDevice SamplerHID;

  void ExecuteCommand(const uint8_t* data, uint16_t len){
    sCommand Cmd;
    memcpy(&Cmd, data, sizeof(sCommand));

    Serial.println("Execution");
    switch(Cmd.m_MsgType){
      default:
        {
          sAnswerStandard Answ;
          Answ.m_SubSystem  = Cmd.m_SubSystem;
          Answ.m_Cmd        = Cmd.m_Cmd;
          Answ.m_UnknownMsg = Cmd.m_MsgType;
          SamplerHID.SendBuffer((uint8_t*)&Answ, sizeof(Answ) );
        }
        break;
    }
    Serial.println("Answer");
  }  
#endif


void setup() {
  Serial.begin(115200); delay(1000);

  #if defined(USE_HID_SAMPLER)
    SamplerHID.Setup();
    SamplerHID.EnableLog(false);
    SamplerHID.SetParser(ExecuteCommand);
/*
Alla ricezione di un messaggio viene eseguito 
  SamplerHIDDevice::_onOutput riceve tutti i messaggi, per debug ne fa un'analisi semplice
    Se impostato viene chiamato m_cbParser/HidParser che trasforma i dati in sCommand e chiama
    ExecuteCommand per poi rispondere con
    SendPhrase
  //---------------------------------------------------------------------------------
  SamplerHIDDevice::_onOutput (se m_MsgType, al momento sempre) chiama ExecuteCommand passando il buffer crudo
  Questo deve smistare verso un esecutore custom analizzando il primo carattere
  Ogni esecutore ha una propria risposta (eventualmente più risposte intermedie)
    Le risposte possono essere definitive o di tipo "heartBit"
  Se non c'è stata risposta ne viene data una standard.
  //---------------------------------------------------------------------------------
  Risposta Standard:
    Ora Inizio
    Ora Fine
    Risultato Ok
*/


  #endif
}

void AlwaysRun(void){
  yield();
}

#if defined(USE_HID_SAMPLER)

#endif

void loop() {
  AlwaysRun();
}
