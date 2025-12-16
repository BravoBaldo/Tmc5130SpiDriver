
#pragma once
#include <Arduino.h>
#include "TMC5130.h"

extern uint8_t StepperInTest; //TMC5130
extern TMC5130 Steppers[];

typedef struct {
  char* Label;
  std::function<void(void)> MenuFunc;
}sMenu;

int   mnuGetChoice  (uint8_t Size);
void  ClearScreen   (void);                   //TMC5130
int   ShowMenu      (char* Menu[], uint8_t Size);
int   ShowMenu      (sMenu Menu[], uint8_t Size);
