#include <SPI.h>
#include "TMC5130.h"
#include "TMC5130_Display.h"
//#include "ansi.h"     //https://github.com/RobTillaart/ANSI
#include "TCA9555.h"  //https://github.com/RobTillaart/TCA9555

#define  I2C_SDA        16     //pin21 i2c serial data
#define  I2C_SCK        17     //pin47 i2c serial clock
#define EXPANDER_ADDR_A	0x20  //I/O port expander A

#define SPI_CS          5        //SPI's Chip Select
#define SPI_FREQ        4000000   //4000000

TCA9555 EXPANDER_A(EXPANDER_ADDR_A);

void  SpiEnableStepper_A(bool en) {
//  digitalWrite(SPI_CS, en?LOW:HIGH);
  EXPANDER_A.write1(9, en?0:1);
}

void  SpiEnableStepper_B(bool en) {
  EXPANDER_A.write1(8, en?0:1);
}
void  initPinCS(uint8_t csPin){ pinMode(csPin, OUTPUT); digitalWrite(csPin, HIGH); }

ANSI ansi(&Serial);

TMC5130 stepper_A(SPI, SpiEnableStepper_A, 4000000);
TMC5130 stepper_B(SPI, SpiEnableStepper_B, 4000000);


void setup() {
  Serial.begin(115200);

  //Expanders initializations
  Wire.begin(I2C_SDA, I2C_SCK, 400000);
  EXPANDER_A.begin(OUTPUT, 0x0000);  //per default mette tutti output
  EXPANDER_A.write16(0xFFFF); //1 means OFF!!!!

  initPinCS(SPI_CS);
  stepper_A.Init_01();
/*
  stepper_A.setCurrent(20, 8, 8);
  stepper_A.setMicrosteps(0);
  stepper_A.enableStealthChop(true);
  stepper_A.setSecondAcceleration(2000);
  stepper_A.setMaxVelocity(400);
  stepper_A.setPosition(0);
*/
  stepper_B.Init_01();

  Serial.println("SPI Full Demo avviata");
//  ansi.clearScreen();

  Serial.print("TCA9555_LIB_VERSION: ");  Serial.println(TCA9555_LIB_VERSION);
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

struct Fsa{
  TMC5130       *stepper;
  uint8_t       fsa;      //Finite State Automata
  bool          dir;      //current direction
  uint8_t       ms;       //Current MicroStep
  int32_t       Steps;    //Current Target
  unsigned long Delay;    //Timer 
  unsigned long MaxWait;  //Time to wait 
  bool          Waiting;  //Flag for waiting
  bool          Laps;     //false=0...Steps, true=-Steps...Steps
}fsa_A={&stepper_A, 0, true,  0, 0, 0, 1000, false, false},
 fsa_B={&stepper_B, 0, false, 3, 0, 0, 2000, false, true };

void loop_Gen(struct Fsa &f){
 switch(f.fsa){
    
    case 0:   //Avvia Stepper
      f.Steps =  f.stepper->Init_MicroSteps(f.ms);
      if(f.Laps)
        f.stepper->moveTo((f.dir ? -f.Steps : f.Steps));
      else
        f.stepper->moveTo((f.dir ? 0 : f.Steps));
      f.fsa++;
      break;

    case 1:   //Aspetta Target
      {
        uint8_t St_A = f.stepper->GetSpiStatus();
        if(St_A&0x20){              //Stepper a destinazione
          f.fsa++;
        }
      }
      break;

    case 2: //Avvia Timer
      f.Delay = millis();
      f.fsa++;
      break;

    case 3: //Aspetta Timer
      if( (millis()-f.Delay) > 2000){
        f.fsa++;
      }
      break;

    case 4: //Cambia Marcia e direzione
      f.dir = !f.dir;
      if(!f.dir){
        f.ms++; if(f.ms>8) f.ms = 0;
      }
      f.fsa = 0;
      break;
    default: f.fsa = 0; break;
  }
}

void loop(void){
  unsigned long time = millis();
  loop_Gen(fsa_A);  //  loop_A(fsa_A);
  loop_Gen(fsa_B);  //loop_B(fsa_B);

  uint8_t St = fsa_A.stepper->GetSpiStatus();
  ansi.gotoXY(1, 1);
    ansi.print("Stepper A: ");
    ansi.print(" Ic Vers.: ");  ansi.print(fsa_A.stepper->getIcVersion(), HEX);
    ansi.print(" MicroSteps: "); ansi.print(fsa_A.stepper->getMicrosteps());
    ansi.print(" CHOPCONF: ");  ansi.print(fsa_A.stepper->readReg(TMC5130::CHOPCONF), HEX);
    GenShowReg(ShowAsSpiStatus, St, 1, 5, true, true);
    ShowActuals(fsa_A.stepper, 1, 20);

  St = fsa_B.stepper->GetSpiStatus();
  ansi.gotoXY(1, 2);
    ansi.print("Stepper B: ");
    ansi.print(" Ic Vers.: ");  ansi.print(fsa_B.stepper->getIcVersion(), HEX);
    ansi.print(" MicroSteps: "); ansi.print(fsa_B.stepper->getMicrosteps());
    ansi.print(" CHOPCONF: ");  ansi.print(fsa_B.stepper->readReg(TMC5130::CHOPCONF), HEX);
    GenShowReg(ShowAsSpiStatus, St, 30, 5);
    ShowActuals(fsa_B.stepper, 30, 20);

  ansi.gotoXY(1, 4);
    ansi.print(millis() - time);

}

void loopQQ() {
  static bool     dir = false;
  static uint8_t  ms = 0;
  int32_t Steps     = stepper_A.Init_MicroSteps(ms);
  //CalcTime(&stepper_A, ms, (dir ? 0 : Steps));  //shows the time for each run 

  //stepper_A.moveTo((dir ? -Steps : Steps));
  stepper_A.moveTo((dir ? 0 : Steps));
  ShowWaiting( &stepper_A, 5000, false);
  //delay(5000);
  //bboard.ShowWaitingX(&stepper_A, 5000, false);
  dir = !dir;
  if(!dir){
    ms++; if(ms>8) ms=0;
  }
}
