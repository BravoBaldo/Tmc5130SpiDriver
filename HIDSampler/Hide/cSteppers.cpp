#include "cSteppers.h"

  #define SPI_SCK   41
  #define SPI_MISO  42
  #define SPI_MOSI  40
  #define SPI_FREQ  4000000
  StepperAnswer GenAnswer;
  
void  SpiEnableSteppers(uint8_t csPin, bool en) { 
	Expanders[1].write1(csPin, en?0:1); 
}  //Callback for Chip-Select through expander


cSteppers::cSteppers() : 
  Steppers {
    TMC5130(SPI,  6,  7, SpiEnableSteppers, SPI_FREQ, "Motor A: Up/Dn"),
    TMC5130(SPI,  4,  5, SpiEnableSteppers, SPI_FREQ, "Motor B: Left/Right"),
    TMC5130(SPI,  2,  3, SpiEnableSteppers, SPI_FREQ, "Motor C: Syringe/Diluter"),
    TMC5130(SPI, 12, 13, SpiEnableSteppers, SPI_FREQ, "Motor D: Depositor"),
    TMC5130(SPI, 10, 11, SpiEnableSteppers, SPI_FREQ, "Motor E"),
    TMC5130(SPI,  8,  9, SpiEnableSteppers, SPI_FREQ, "Motor F")
  } 
{
	for(int i=0; i<wxSIZEOF(Steppers); i++){
		TMC5130_Init_00(Steppers[i]);
		Enable(i, false);
		Expanders[1].write1(Steppers[i].getcsPinAddress(), 1);  //0 Activate	
		
	}
}

void cSteppers::Enable(uint8_t MotIdx, bool En){
	Expanders[1].write1(Steppers[MotIdx].getcePinAddress(), En?0:1);  //0 Activate	
}

void cSteppers::InitFreeRotate(uint8_t MotIdx, bool Direction){
	Enable(MotIdx, true);
	TMC5130_Init_00(Steppers[MotIdx]);
	{
	  TMC5130::SwMode sw_mode = Steppers[MotIdx].getSwMode();  //SW_MODE
	  sw_mode.swap_lr          = Direction?1:0;    //1: Swap the left and the right reference switch input REFL and REFR
	  sw_mode.stop_l_enable    = 1;	   //1: Enables automatic motor stop during active left reference switch input
	  sw_mode.pol_stop_l       = 1;    //0=non-inverted, high active, 1=inverted, low active

	  sw_mode.stop_r_enable    = 1;    //1: Enables automatic motor stop during active right reference switch input
	  sw_mode.pol_stop_r       = 1;    //0=non-inverted, high active, 1=inverted, low active
	  sw_mode.sg_stop          = 0;    //1: Enable stop by StallGuard2 (also available in DcStep mode). Disable to release motor after stop event.           
	  sw_mode.en_softstop      = 0;    //0: Hard stop 1: Soft stop    
	  
	  Steppers[MotIdx].setSwMode(sw_mode);
	}
	Steppers[MotIdx].getGstat();
	Steppers[MotIdx].setCurrent(7, 1, 1);  
	SetFreeRunning(Steppers[MotIdx], 2, 0, Direction);
}

void cSteppers::Setup(void){
  {
  //Stepper 2,  Da Sx a Dx
	bool Direction = true;
    TMC5130::SwMode sw_mode = Steppers[2].getSwMode();
    sw_mode.swap_lr       = Direction?1:0;  //
    sw_mode.stop_l_enable = 1;
    sw_mode.stop_r_enable = 1;
    sw_mode.pol_stop_l    = 1;
    sw_mode.pol_stop_r    = 1;
    sw_mode.en_softstop   = 0;
	sw_mode.sg_stop       = 0;    //1: Enable stop by StallGuard2 (also available in DcStep mode). Disable to release motor after stop event.           
    Steppers[2].setSwMode(sw_mode);
    Steppers[2].setMaxSteps(-550);
    Steppers[2].setMicrosteps(7);
    Steppers[2].setGoHomeVelocity(200);
    //Reset: 100,300,300
    //GoEnd: 100,300,-450

    Steppers[2].setResets(); //Read current settings and use them as defaults
  }
	Steppers[0].Exec_Stop();
	Steppers[1].Exec_Stop();
	Steppers[2].Exec_Stop();

	InitFreeRotate(0, true);
	InitFreeRotate(1, false);
	//InitFreeRotate(2, false);
  
}

void cSteppers::TestAllSteppers(void){
	bool AllOk=true;
	for(int i=0; i<wxSIZEOF(Steppers); i++){
	  bool test = Steppers[i].IsConnected();
	  Serial.printf("\t%s %s\n", Steppers[i].GetName(), test?"Ok":"KO");
	  if(test==false)
		AllOk=false;
	}
	if(AllOk==false){
	  Serial.println("Not all steppers are OK.\n");
	  while (1);
	}else
	  Serial.println("All Steppers are OK");  
}

void cSteppers::Loop(void){
  for(byte i=0; i<wxSIZEOF(Steppers); i++){
    Steppers[i].FSA_loop();
  }	
}

void cSteppers::PrintFSAInfo(void){
	Serial.printf("FSA: %4s %4s %4s %4s\n"
		, Steppers[0].IsFSAFree()?"Free":"Busy"
		, Steppers[1].IsFSAFree()?"Free":"Busy"
		, Steppers[2].IsFSAFree()?"Free":"Busy"
		, Steppers[3].IsFSAFree()?"Free":"Busy"
	);
}
