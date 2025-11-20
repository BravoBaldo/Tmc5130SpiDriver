
#pragma once
#include <Arduino.h>
#include "TMC5130.h"
#include "ansi.h"     //https://github.com/RobTillaart/ANSI

extern ANSI ansi;

  typedef enum  : uint8_t {
    ShowAsSwitch,
    ShowAsRamp,
    ShowAsSpiStatus,
    ShowAsInputs,
    ShowAsGStatus,
  }ShowAsTyp;

void GenShowReg     (ShowAsTyp Typ, uint32_t Val, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);

void      ShowRampStatus (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
uint8_t   ShowSpiStatus  (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
uint8_t   ShowInputs     (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowGStatus    (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
uint16_t  ShowSwitchMode (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowSwitchMode2(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowActuals    (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowActualsFast(TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowCurrents   (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);
void      ShowDrvStatus  (TMC5130 *stp, uint8_t Col, uint8_t Row, bool Vert=true, bool ShowTitle=true);

void      ShowWaiting    (TMC5130 *stp, const float waiting, bool WaitStop=true);


class Printer {
public:
//  Printer();
  Printer(Print & print) : print_ptr_(&print) {};

  void readAndPrintGconf(TMC5130 &stepper);
  void printRegister(TMC5130::Gconf gconf);

  void readAndPrintPwmScale(TMC5130 &stepper);
  void printRegister(TMC5130::PwmScale & pwm_scale);

private:
  Print * print_ptr_;
  void printRegisterPortion(const char * str, uint32_t value, int base=DEC);
};
