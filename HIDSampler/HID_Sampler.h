#pragma once
#include <Arduino.h>
/*
                                                                                1) "USB CDC On Boot:"...........: "Disabled".
                                                                                2) "USB DFU On Boot:"...........: "Disabled".
                                                                                3) "USB Firmware MSC On Boot:"..: "Disabled".
                                                                                4) "Upload Mode:................: "USB-OTG CDC (TynyUSB)"
                                                                                5) "USB Mode:"..................: "USB-OTG (TynyUSB)".
*/
#ifndef ARDUINO_USB_MODE
  #error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
  #error This sketch should be used when USB is in OTG mode (USB Mode: "Hardware CDC and JTAG" is NOT correct for custom HID)
#endif

// "USB CDC On Boot" 
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || ARDUINO_USB_CDC_ON_BOOT==1
  #error "USB CDC On Boot" must be 'Disabled'
#endif
/*
Corrisponde alla macro ARDUINO_USB_CDC_ON_BOOT. 
Cosa fa: Se Enabled, il firmware abilita automaticamente la porta seriale virtuale (CDC - Communications Device Class) non appena la scheda si accende.
Perché serve: Permette di visualizzare i messaggi del Serial.print() nel Monitor Seriale fin dall'avvio senza configurazioni manuali complesse nel codice. Se disabilitato, la porta seriale potrebbe non apparire sul PC a meno che non venga inizializzata esplicitamente nel setup(). 

USB Mode: Definisce come l'hardware USB viene gestito.
  USB-OTG (Hardware): Utilizza il controller USB interno per emulare dispositivi come tastiere o mouse (HID).
  Hardware CDC and JTAG: Utilizza il modulo hardware dedicato per la seriale e il debug, spesso più stabile per il caricamento del codice.
USB DFU on Boot: Corrisponde alla macro ARDUINO_USB_DFU_ON_BOOT.
  Funzione: Abilita il Device Firmware Upgrade all'avvio.
  Utilizzo: Permette di caricare il firmware tramite lo standard DFU, utile se il caricamento standard via seriale fallisce o se si vuole usare un programmatore universale.
USB JTAG debug unit:
  Funzione: Abilita il modulo di debug hardware integrato.
  Utilizzo: Consente di utilizzare un debugger (come quello integrato in Arduino IDE 2.0) per mettere in pausa il codice, analizzare i registri e trovare bug complessi senza usare solo i Serial.print. 

Nota Bene: Se abiliti USB CDC on Boot ma il tuo codice contiene while (!Serial);, la scheda rimarrà bloccata finché non aprirai effettivamente il Monitor Seriale sul PC. 
*/



//#define USE_REPORT_ID

#include <USB.h>
#include <USBHID.h>
#include <functional>  // for std::function

#define UVID 0x6666
#define UPID 0x0827 

typedef std::function<void(const uint8_t* data, uint16_t len)> HID_PARSER_CB;

class SamplerHIDDevice : public USBHIDDevice {
  uint8_t m_OutBuffer[0x40];  //See descriptor
  bool    m_LogRx = true;
  HID_PARSER_CB m_cbExec = nullptr;
public:
  SamplerHIDDevice(void);

  uint16_t _onGetDescriptor(uint8_t *buffer);
  void _onOutput(uint8_t report_id, const uint8_t* data, uint16_t len);

  void begin(void);
  void SendBuffer(uint8_t* Buffer, size_t len, uint8_t ReportId=0);

  void  EnableLog(bool En) { m_LogRx = En;}
  void  SetParser(HID_PARSER_CB callback) {m_cbExec = callback; }
  
  void Setup(void);
};
