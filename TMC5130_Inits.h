
#pragma once
#include <Arduino.h>
#include "TMC5130.h"

void TMC5130_Init_00  (TMC5130 &stepper);
void TMC5130_Init_01  (TMC5130 &stepper);
void TMC5130_Init_02  (TMC5130 &stepper, bool Start=false); //Ready for run @ 1 rps
void SetFreeRunning   (TMC5130 &stepper, uint8_t SpeedFor1RPS=1);
void InitTestStall    (TMC5130 &stepper);