#pragma once
#include <Arduino.h>
#include <TCA9555.h>          //I/O Expanders

#include "TMC5130.h"
#include "TMC5130_Inits.h"

#include "../SysSampler.h"

extern TCA9555   Expanders[];

void  SpiEnableSteppers(uint8_t csPin, bool En);



class cSteppers {
	TMC5130 Steppers[6];
public:
  cSteppers(void);
  void TestAllSteppers(void);
  void InitFreeRotate(uint8_t MotIdx, bool Direction);
  void Setup(void);
  void Loop(void);
  void Enable(uint8_t MotIdx, bool En);
  bool Halt(uint8_t MotIdx, uint16_t a)		{ return Steppers[MotIdx].Exec_Stop(a); };
  bool WaitEnd	(uint8_t MotIdx)			{ return Steppers[MotIdx].IsFSAFree(); }  //see Exec_WaitOperations
  bool WaitStop	(uint8_t MotIdx)			{ return Steppers[MotIdx].FSA_WaitStop(); }
  bool GoTo(uint8_t MotIdx, uint16_t A, uint32_t V, int32_t  S) { return Steppers[MotIdx].Exec_GoTo(A,V,S);}
  //bool setCurrents(uint8_t MotIdx, uint8_t irun, uint8_t ihold, uint8_t holdDelay)	{ return Steppers[MotIdx].Exec_setCurrent(irun, ihold, holdDelay);}
  bool setCurrents(uint8_t MotIdx, uint8_t irun, uint8_t ihold, uint8_t holdDelay)	{ Steppers[MotIdx].setCurrent(irun, ihold, holdDelay);return true;}
  
  //StepperAnswer GetAnswer(void);
  void PrintFSAInfo(void);
};