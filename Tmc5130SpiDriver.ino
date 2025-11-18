#include <SPI.h>
#include "TMC5130.h"
#include "TMC5130_Display.h"
#include "TCA9555.h"  //https://github.com/RobTillaart/TCA9555
#include "TMC5130_Inits.h"

#define wxSIZEOF(a) (sizeof(a)/sizeof(a[0]))

#define I2C_SDA        16     //pin21 i2c serial data
#define I2C_SCK        17     //pin47 i2c serial clock
#define EXPANDER_ADDR_A	0x20  //I/O port expander A

//#define SPI_CS          5        //SPI's Chip Select
#define SPI_FREQ        4000000   //4000000

enum : uint8_t {
  StepperNameA = 0,
  StepperNameB,
  StepperName_Count  //Must be last
}StepperNames;


enum : uint8_t {
  ExpanderA = 0,
  Expander_Count  //Must be last
}Exanders;


TCA9555 EXPANDERS[]{  TCA9555(EXPANDER_ADDR_A),
                  };
ANSI    ansi(&Serial);

void  SpiEnableSteppers(uint8_t csPin, bool en) { EXPANDERS[ExpanderA].write1(csPin, en?0:1); }  //Callback for Chip-Select through expander

//AAA: Maybe SPI_FREQ must be unique!!!
TMC5130 Steppers[]={TMC5130(SPI, 9, SpiEnableSteppers, SPI_FREQ, "Motor A"),
                    TMC5130(SPI, 8, SpiEnableSteppers, SPI_FREQ, "Motor B"),
                };


void setup() {
  Serial.begin(115200);

  //Expanders initializations
  Wire.begin(I2C_SDA, I2C_SCK, 400000);
  EXPANDERS[ExpanderA].begin(OUTPUT, 0x0000);  //per default mette tutti output
  EXPANDERS[ExpanderA].write16(0xFFFF); //1 means OFF!!!!

//pinMode(SPI_CS, OUTPUT); digitalWrite(SPI_CS, HIGH);  //Init for ChipSelect on board

  for(int i=0; i<wxSIZEOF(Steppers); i++){
    TMC5130_Init_01(Steppers[i]);
  }

  SetFreeRunning(Steppers[1], 1); //Free running @ ??? Rotation Per Second
  Steppers[1].writeReg(TMC5130::IHOLD_IRUN,  0x00070101);  //
}

void CalcTime(TMC5130 *stp, uint8_t ms, int32_t Steps){ //shows the time for each run
  Serial.print(ms);  Serial.print(":");
  unsigned long time = millis();
  stp->moveTo(Steps);
  do{
    stp->genSpiFunct(TMC5130::GCONF, 0, true);  //dummy reading
  }while( !(stp->GetSpiStatus() & 0x20));
  Serial.println(millis()-time);
}

struct Fsa {  //Finite State Machine for Motors
  TMC5130       *stepper;
  uint8_t       fsa;      //Finite State Automata
  bool          dir;      //current direction
  uint8_t       ms;       //Current MicroStep
  int32_t       Steps;    //Current Target
  unsigned long Delay;    //Timer 
  unsigned long MaxWait;  //Time to wait 
  bool          Waiting;  //Flag for waiting
  bool          Laps;     //false=0...Steps, true=-Steps...Steps
}fsa[]={  {&Steppers[StepperNameA], 0, true,  0, 0, 0, 1000, false, false},
//        {&Steppers[StepperNameB], 0, false, 3, 0, 0, 2000, false, true }
 };

uint8_t loop_Gen(struct Fsa &f){
 switch(f.fsa){
    case 0:   //Start Stepper
      f.Steps =  f.stepper->Init_MicroSteps(f.ms);
      if(f.Laps)
        f.stepper->moveTo((f.dir ? -f.Steps : f.Steps));
      else
        f.stepper->moveTo((f.dir ? 0        : f.Steps));
      f.fsa++;
      break;

    case 1:   //Wait Target
      {
        uint8_t St_A = f.stepper->GetSpiStatus();
        if(St_A&0x20){              //Stepper a destinazione
          f.fsa++;
        }
      }
      break;

    case 2: //Start Timer
      f.Delay = millis();
      f.fsa++;
      break;

    case 3: //Wait Timer
      if( (millis()-f.Delay) > 2000){
        f.fsa++;
      }
      break;

    case 4: //Change micro Stepping and direction
      f.dir = !f.dir;
      if(!f.dir){
        f.ms++; if(f.ms>8) f.ms = 0;
      }
      f.fsa = 0;
      break;

    default: f.fsa = 0; break;
  }
  return f.fsa;
}

void DisplayMotor(TMC5130 &stp, uint8_t idx){
  ansi.gotoXY(1, 1+idx);
    ansi.print(stp.GetName());
    ansi.print(": Ic Ver.: ");    ansi.print(stp.getIcVersion(), HEX);
    ansi.print(" MicroSteps: ");  ansi.print(stp.getMicrosteps());
//  ansi.print(" CHOPCONF: ");    ansi.print(stp.readReg(TMC5130::CHOPCONF), HEX);

    uint8_t Row = 5+idx*16;
    uint8_t St  = stp.GetSpiStatus();
    GenShowReg(ShowAsSpiStatus, St,  1, Row, true, true);
    ShowActualsFast(&stp,           22, Row);
//    ShowSwitchMode(&stp,            51, Row);
//    ShowCurrents(&stp,            51, Row);
    ShowDrvStatus(&stp,            51, Row);
}

void loop(void){
  static unsigned long timeRefresh = millis();
  unsigned long time = millis();

  //--Execute Finite State Machines ---------------------------------------------------------------------
  for(uint8_t i=0; i<wxSIZEOF(fsa); i++){
    loop_Gen(fsa[i]);
  }

  //--Display Results-----------------------------------------------------------------
  for(uint8_t i=0; i<wxSIZEOF(Steppers); i++){
    DisplayMotor(Steppers[i], i);
  }

  //--Display Times -----------------------------------------------------------------
  ansi.gotoXY(1, 4);
    ansi.print(millis() - time);

  unsigned long t = (millis()-timeRefresh);
  //ansi.gotoXY(20, 4); ansi.print(t);
  if(t>10000){
    timeRefresh = millis();
    ansi.clearScreen();
  }
}
