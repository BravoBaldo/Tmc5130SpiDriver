
#pragma once
#include <Arduino.h>
#include "TMC5130.h"

void TMC5130_Init_00      (TMC5130 &stepper);
void TMC5130_Init_DS0     (TMC5130 &stepper); //DataSheet Suggested Init sample
void TMC5130_Init_DS1     (TMC5130 &stepper); //DataSheet Suggested GoTo
void SetPositional        (TMC5130 &stepper, uint8_t SpeedFor1RPS, uint8_t mres);
void SetFreeRunning       (TMC5130 &stepper, uint8_t SpeedFor1RPS=1, uint8_t mres=0, bool Positive=true);
