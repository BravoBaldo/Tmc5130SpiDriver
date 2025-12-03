
#pragma once
#include <Arduino.h>
#include "TMC5130.h"

void TMC5130_Init_00      (TMC5130 &stepper);
void TMC5130_Init_01      (TMC5130 &stepper);
void TMC5130_Init_02      (TMC5130 &stepper, bool Start=false); //Ready for run @ 1 rps
void TMC5130_Init_DS0     (TMC5130 &stepper); //DataSheet Suggested Init sample
void TMC5130_Init_DS1     (TMC5130 &stepper); //DataSheet Suggested GoTo

void SetFreeRunning       (TMC5130 &stepper, uint8_t SpeedFor1RPS=1, uint8_t mres=0);
void SetPositional        (TMC5130 &stepper, uint8_t SpeedFor1RPS, uint8_t mres);

void InitTestStall        (TMC5130 &stepper);
void InitTestStall_Setup  (TMC5130 &stepper);
void beginHomeToStall     (TMC5130 &stepper, TMC5130::HomeParameters home_parameters, TMC5130::StallParameters stall_parameters);
void InitGoto             (TMC5130 &stepper);
