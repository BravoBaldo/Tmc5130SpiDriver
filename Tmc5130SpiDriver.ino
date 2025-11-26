/*
ToDo:
  Eecute an "HomeToStall"
  Check if to use Polidoro's class printer....
  remove TMC5130_RampMode
  unificare Gconf and GconfBits

  Tests
    Seriale
    connessione
    Fermata 'morbida':  Ok StopMotor
    In 'Neutral=Folle'  Ok
    In 'Parking'        Ok
    MotA continuous rotation, MotB two laps forward and two back
*/


#include <SPI.h>
#include "TMC5130.h"
#include "TMC5130_Display.h"
#include "TCA9555.h"          //https://github.com/RobTillaart/TCA9555
#include "TMC5130_Inits.h"

#define I2C_SDA         16     //pin21 i2c serial data
#define I2C_SCK         17     //pin47 i2c serial clock
#define EXPANDER_ADDR_A 0x20  //I/O port expander A

//#define SPI_CS          5        //SPI's Chip Select
#define SPI_FREQ        4000000   //4000000


#define wxSIZEOF(a) (sizeof(a)/sizeof(a[0]))
#define LOOP_DELAY  500
#define PAUSE_DELAY 4000

TMC5130::HomeParameters       home_parameters_chip;
TMC5130::StallParameters      stall_parameters_chip;


//--- List All expanders ------------------------------------------
enum : uint8_t {
  ExpanderA = 0,
  Expander_Count  //Must be last
}Exanders;

TCA9555 EXPANDERS[]{  TCA9555(EXPANDER_ADDR_A),
                  };
//-----------------------------------------------------------------



// --- Setup Steppers -----------------------------------------------------------------
void  SpiEnableSteppers(uint8_t csPin, bool en) { EXPANDERS[ExpanderA].write1(csPin, en?0:1); }  //Callback for Chip-Select through expander

enum : uint8_t {
  StepperNameA = 0,
  StepperNameB,
  StepperName_Count  //Must be last
}StepperNames;

//AAA: Maybe SPI_FREQ must be unique!!!
TMC5130 Steppers[]={TMC5130(SPI, 9, SpiEnableSteppers, SPI_FREQ, "Motor A"),
                    TMC5130(SPI, 8, SpiEnableSteppers, SPI_FREQ, "Motor B"),
                };
#define STEP_STALL StepperNameB //For Stall experiments
//---------------------------------------------------------------------------------------

ANSI    ansi(&Serial);

void setup_Basic() {
  Serial.begin(115200);

  //Expanders initializations
  Wire.begin(I2C_SDA, I2C_SCK, 400000);
  EXPANDERS[ExpanderA].begin(OUTPUT, 0x0000);  //per default mette tutti output
  EXPANDERS[ExpanderA].write16(0xFFFF); //1 means OFF!!!!

  //pinMode(SPI_CS, OUTPUT); digitalWrite(SPI_CS, HIGH);  //Init for ChipSelect on board

  for(int i=0; i<wxSIZEOF(Steppers); i++){
    TMC5130_Init_01(Steppers[i]);
  }

#ifdef QQQQQQQQQQQQQQQQQQ
  Steppers[1].writeReg(TMC5130::IHOLD_IRUN,  0x00070101);  //
//  InitTestStall    (Steppers[0]);
  //Check Communications
  bool AllOk;
  do{
    AllOk = true;
     Serial.println("Check Communication...");
    for(int i=0; i<wxSIZEOF(Steppers); i++){
      Serial.print(i);  Serial.print(") '"); Serial.print(Steppers[i].GetName());  Serial.print("' ");
      if(Steppers[i].communicating())
        Serial.println("is ok");
      else{
            Serial.println(Steppers[i].communicating() ? "is ok" : "NOT OK");
            AllOk = false;
      }
    }
    delay(1000);
  }while(!AllOk);

  TMC5130::ControllerParameters OldRegs;
  Steppers[STEP_STALL].cacheControllerSettings(OldRegs);
  Steppers[STEP_STALL].beginRampToZeroVelocity();
  while (not Steppers[STEP_STALL].zeroVelocity()) {
    Serial.println("Waiting for zero velocity.");
    delay(1000);
  }
  Steppers[STEP_STALL].writeControllerParameters(OldRegs);

  InitTestStall_Setup  (Steppers[STEP_STALL]);
    home_parameters_chip.run_current        = 7;
    home_parameters_chip.hold_current       = 3;
    home_parameters_chip.target_position    = 4294152396;
    home_parameters_chip.velocity           = 227862;
    home_parameters_chip.acceleration       = 248;
    home_parameters_chip.zero_wait_duration = 2353;

    stall_parameters_chip.stall_guard_threshold = 3;
    stall_parameters_chip.cool_step_threshold   = 147;

  //ShowReadableRegisters(Steppers[STEP_STALL]);

#endif
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
          {&Steppers[StepperNameB], 0, false, 3, 0, 0, 2000, false, true }
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


void loopAAA(void){
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

void loopWWW(void){
  Serial.println("Waiting...");
  delay(PAUSE_DELAY);

  Serial.println("Homing to stall...");
  beginHomeToStall(Steppers[STEP_STALL], home_parameters_chip, stall_parameters_chip);
  int32_t actual_position_real;
  while (not Steppers[STEP_STALL].homed()) {
    // stepper.printer.readAndPrintDrvStatus();
    int32_t actual_position_chip = Steppers[STEP_STALL].getPosition(); //stepper.controller.readActualPosition();
    actual_position_real = actual_position_chip;  //ToDo
/*
    int32_t Converter::positionChipToReal(int32_t position_chip) {
      return position_chip / (int32_t)converter_parameters_.microsteps_per_real_position_unit;
    }
    actual_position_real = stepper.converter.positionChipToReal(actual_position_chip);
*/
    Serial.print("homing...");
    Serial.print("actual position (radians): ");  Serial.println(actual_position_real);
    Serial.print("stall guard result: "); Serial.println(Steppers[STEP_STALL].readStallGuardResult());
    Serial.print("stall guard threshold: ");
    Serial.println("???");//Serial.println(stall_parameters_real.stall_guard_threshold);  //ToDo
    delay(LOOP_DELAY);
  }
  Steppers[STEP_STALL].endHome();//  stepper.endHome();

  Serial.println("Homed!");
  Serial.print("actual_position_real: "); Serial.println(actual_position_real);
  //Serial.print("home target position: "); Serial.println(home_parameters_real.target_position);

  Serial.println("Waiting...");
  delay(PAUSE_DELAY);

  #define MOVE_POSITION  110  // radians
  int32_t target_position_chip = MOVE_POSITION*8149;  //stepper.converter.positionRealToChip(MOVE_POSITION); //110*8149 radians * microsteps_per_real_position_unit
  Steppers[STEP_STALL].writeReg(TMC5130::XTARGET,  target_position_chip); //stepper.controller.writeTargetPosition(target_position_chip);
  Serial.print("Moving to another position (radians): "); Serial.print(MOVE_POSITION);  Serial.println("...");

  while (not Steppers[STEP_STALL].positionReached()) {
    // stepper.printer.readAndPrintRampStat();
    // stepper.printer.readAndPrintDrvStatus();
    int32_t actual_position_chip = Steppers[STEP_STALL].getPosition();   //stepper.controller.readActualPosition();
    int32_t actual_position_real = actual_position_chip/8149;         //stepper.converter.positionChipToReal(actual_position_chip);
    Serial.print("actual position (radians): ");Serial.println(actual_position_real);
    Serial.print("stall_guard_result: ");
    Serial.println(Steppers[STEP_STALL].readStallGuardResult());
    delay(LOOP_DELAY);
  }
  Serial.println("Target position reached!");
  delay(PAUSE_DELAY);

  Serial.println("--------------------------");
  delay(LOOP_DELAY);
}

void setup() {
  setup_Basic();
  delay(2000);

  //Init Steppers to default
  for(int i=0; i<wxSIZEOF(Steppers); i++){
    ShowReadableRegisters(Steppers[i]);
    while (!Steppers[i].IsConnected()) {
      Serial.print("ERROR: Stepper '");
      Serial.print(Steppers[i].GetName());
      Serial.println("': No communication detected, check motor power and connections.");
      delay(500);
    }
    TMC5130_Init_00(Steppers[i]);
  }
  SetFreeRunning(Steppers[StepperNameA], 1,0); //Free running @ ??? Rotation Per Second
  SetFreeRunning(Steppers[StepperNameB], 2,8); //Free running @ ??? Rotation Per Second
}

void ShowMenu(){
  static char* Menu[]={"","Hard Stop", "Soft Stop", "Neutral", "Parking", "Free running", "Reverse"};
  Serial.println("\n\n");
  for(int i=1; i<wxSIZEOF(Menu);i++){
    Serial.print(i);  Serial.print(".");  Serial.println(Menu[i]);
  }

  Serial.print("\nYour choice:");
  while (Serial.available() < 2) ;
  int menuChoice = Serial.parseInt();
  Serial.print(menuChoice);
  if(menuChoice<wxSIZEOF(Menu)){
    Serial.print(": ");
    Serial.print(Menu[menuChoice]);
  }else
    Serial.println(" Invalid choice");

  switch(menuChoice){
    case 1: Steppers[1].StopMotor(1000);  break;
    case 2: Steppers[1].StopMotor(1);     break;
    case 3: Steppers[1].setNeutral();     break;
    case 4: Steppers[1].setParking();     break;
    case 5: Steppers[1].setCurrent(7, 1, 1);  SetFreeRunning(Steppers[StepperNameB], 2,8);  break;
    case 6: Steppers[1].setRampMode(TMC5130::VelocityNegativeMode); break;

    default: Serial.println("Please choose a valid selection"); return;
  }
}
void loop(void){
      ShowMenu();
 }
 