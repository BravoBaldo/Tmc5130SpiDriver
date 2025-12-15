/*
ToDo:
  GoTo and wait target
  Eecute an "HomeToStall"
  Check if to use Polidoro's class printer....
  Implementare gli algoritmi descritti nel datasheet

Tests
  Seriale
  connessione
  Fermata 'morbida':  Ok StopMotor
  In 'Neutral=Folle'  Ok
  In 'Parking'        Ok
  MotorA continuous rotation, MotB two laps forward and two back
  GoTo... steps

Done:
  remove TMC5130_RampMode
  Introduce ShadowRegs (see DriverParameters)
  Le inizializzazioni del Datasheet
  Eliminare StepperNames
  unificare Gconf and GconfBits
  Rendere Regs Private
*/


#include <SPI.h>
#include "TMC5130.h"
#include "TMC5130_Display.h"
#include "TCA9555.h"          //https://github.com/RobTillaart/TCA9555
#include "TMC5130_Inits.h"
#include "StringSplitter.h" //https://github.com/aharshac/StringSplitter
#include "TMC5130_Menu.h"

#define I2C_SDA         16     //pin21 i2c serial data
#define I2C_SCK         17     //pin47 i2c serial clock
#define EXPANDER_ADDR_A 0x20  //I/O port expander A

//#define SPI_CS          5        //SPI's Chip Select
#define SPI_FREQ        4000000   //4000000

#define LOOP_DELAY  500
#define PAUSE_DELAY 4000

TMC5130::HomeParameters       home_parameters_chip;
TMC5130::StallParameters      stall_parameters_chip;


//--- List All expanders ------------------------------------------

enum : uint8_t {
  ExpanderA = 0,
  Expander_Count  //Must be last
}Expanders;

TCA9555 EXPANDERS[]{
                      TCA9555(EXPANDER_ADDR_A),
                  };
//-----------------------------------------------------------------



// --- Setup Steppers -----------------------------------------------------------------
void  SpiEnableSteppers(uint8_t csPin, bool en) { EXPANDERS[ExpanderA].write1(csPin, en?0:1); }  //Callback for Chip-Select through expander

//AAA: Maybe SPI_FREQ must be unique!!!
TMC5130 Steppers[]={TMC5130(SPI,  9, SpiEnableSteppers, SPI_FREQ, "Motor A: Up/Dn"),
                    TMC5130(SPI,  8, SpiEnableSteppers, SPI_FREQ, "Motor B:"),
                    TMC5130(SPI, 10, SpiEnableSteppers, SPI_FREQ, "Motor C"),
//                    TMC5130(SPI, 11, SpiEnableSteppers, SPI_FREQ, "Motor D"),
                };
#define STEP_STALL 0 //For Stall experiments
//---------------------------------------------------------------------------------------

ANSI    ansi(&Serial);

uint8_t StepperInTest = 0;

void ShowReadableRegistersAll(void);


void setup_Basic() {
  Serial.begin(115200);
  delay(1000);

  //Expanders initializations
  Wire.begin(I2C_SDA, I2C_SCK, 400000);
  
  EXPANDERS[ExpanderA].begin(OUTPUT, 0x0000);  //per default mette tutti output
  EXPANDERS[ExpanderA].write16(0xFFFF); //1 means OFF!!!!

  //pinMode(SPI_CS, OUTPUT); digitalWrite(SPI_CS, HIGH);  //Init for ChipSelect on board
  
  Serial.println("Just turned On registers");
  Serial.println("Just turned On registers");
  Serial.println("Just turned On registers");
  ShowReadableRegistersAll();

  for(int i=0; i<wxSIZEOF(Steppers); i++){
    TMC5130_Init_00(Steppers[i]);
  }

}

void CalcTime(TMC5130 *stp, uint8_t ms, int32_t Steps){ //shows the time for each run
  Serial.print(ms);  Serial.print(":");
  unsigned long time = millis();
  //stp->moveTo(Steps);
  stp->setTarget(Steps);
  stp->setRampMode(TMC5130::PositionMode);
  do{
    //stp->getGconf();//genSpiFunct(TMC5130::GCONF, 0, true);  //dummy reading
  }while( !(stp->GetSpiStatus().position_reached));
  Serial.println(millis()-time);
}

struct Fsa {  //Finite State Machine for Motors
  TMC5130       *stepper = nullptr;
  uint8_t       fsa;      //Finite State Automata
  bool          dir;      //current direction
  uint8_t       ms;       //Current MicroStep
  int32_t       Steps;    //Current Target
  unsigned long Delay;    //Timer
  unsigned long MaxWait;  //Time to wait 
  bool          Waiting;  //Flag for waiting
  bool          Laps;     //false=0...Steps, true=-Steps...Steps
}fsa[wxSIZEOF(Steppers)];

uint8_t loop_Gen(struct Fsa &f){
 switch(f.fsa){
    case 0:   //Start Stepper
      f.Steps =  f.stepper->Init_MicroSteps(f.ms);
      if(f.Laps)
        f.stepper->setTarget((f.dir ? -f.Steps : f.Steps));
      else
        f.stepper->setTarget((f.dir ? 0        : f.Steps));
      f.fsa++;
      break;

    case 1:   //Wait Target ToDo: Add Timeout
        if(f.stepper->GetSpiStatus().position_reached){              //Stepper a destinazione
          f.fsa++;
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
    uint8_t St  = stp.GetSpiStatus().bytes;
    GenShowReg(ShowAsSpiStatus, St,  1, Row, true, true);
//ToDo    ShowActualsFast(&stp,           22, Row);
//    ShowSwitchMode(&stp,            51, Row);
//    ShowCurrents(&stp,            51, Row);
    ShowDrvStatus(&stp,            51, Row);
}


void loop_Fsa(uint8_t LogTo=0){ //0=None, 1=Serial, 2= Ansi
  static unsigned long timeRefresh = millis();
  unsigned long time = millis();
  unsigned long t;
  //--Execute Finite State Machines ---------------------------------------------------------------------
  for(uint8_t i=0; i<wxSIZEOF(fsa); i++){
    loop_Gen(fsa[i]);
  }
  switch(LogTo){
    case 1:
      Serial.print("F.S.A: ");
      for(uint8_t i=0; i<wxSIZEOF(fsa); i++){
        uint8_t r = loop_Gen(fsa[i]);
        Serial.printf("%d ", r);
      }
      Serial.println("");

    break;
    case 2:
      //--Display Results-----------------------------------------------------------------
      for(uint8_t i=0; i<wxSIZEOF(Steppers); i++){
        DisplayMotor(Steppers[i], i);
      }

      //--Display Times -----------------------------------------------------------------
      ansi.gotoXY(1, 4);
        ansi.print(millis() - time);

      t = (millis()-timeRefresh);
      //ansi.gotoXY(20, 4); ansi.print(t);
      if(t>10000){
        timeRefresh = millis();
        ansi.clearScreen();
      }
      break;
    default:
      break;
  }
}

void ShowReadableRegistersAll(void){
  Serial.println("---Readable Registers---");
  Serial.printf("      : ");
    for(int j=0; j<wxSIZEOF(Steppers); j++){
      Serial.printf(" %8s ", Steppers[j].GetName());
    }
    Serial.printf("<--\n");

  for(int i=0; i<wxSIZEOF(TMC5130::RegsReadable); i++){
    Serial.printf("%02d) %02X: ", i, TMC5130::RegsReadable[i]);
    for(int j=0; j<wxSIZEOF(Steppers); j++){
      Serial.printf(" %08X ", Steppers[j].readReg(TMC5130::RegsReadable[i]));
    }
    Serial.printf("<--\n");
  }
  Serial.println("-------------------------------\n");
}

void setup() {
  setup_Basic();  //Serial, Expander(s), Motor(s)
  delay(2000);

  ShowReadableRegistersAll();
  //Init Steppers to default
  for(int i=0; i<wxSIZEOF(Steppers); i++){
    //ShowReadableRegisters(Steppers[i]);
    //Check Motor Power
    while (!Steppers[i].IsConnected()) {
      Serial.print("ERROR: Stepper '");
      Serial.print(Steppers[i].GetName());
      Serial.println("': No communication detected, check motor power and connections.");
      delay(500);
    }
    TMC5130_Init_00(Steppers[i]);

    fsa[i] = {&Steppers[i], 0,  false,  3,  0,    0,    1000,   false,  false};
  }

  for(int i=0; i<wxSIZEOF(Steppers); i++){
    //SetFreeRunning(Steppers[i], 2,8); //Free running @ ??? Rotation Per Second
    SetPositional(Steppers[i], 2,8);
  }

  {
    //Motore 0: Con valori positivi sale, sopra 0, scende fino a -4040
  //  Steppers[0].writeSwapRL(true);                          //SW_MODE
    TMC5130::SwMode sw_mode = Steppers[0].getSwMode();
    sw_mode.swap_lr       = 1;
    sw_mode.stop_l_enable = 1;
    sw_mode.stop_r_enable = 1;
    sw_mode.pol_stop_l    = 0;
    sw_mode.pol_stop_r    = 0;
    sw_mode.en_softstop   = 0;
    Steppers[0].setSwMode(sw_mode);

    Steppers[0].setMaxSteps(-3840);
    Steppers[1].setMicrosteps(8); //Full Step

    //Reset: 100,600,3000     100,600,-1
    //GoEnd: 100,2000,-3840
    //Prosegue verso numeri negativi
  }

  {
  //Stepper 1, torna indietro se positivo
    TMC5130::SwMode sw_mode = Steppers[1].getSwMode();
    sw_mode.swap_lr       = 0;  //con 1 si chiude R, con 0 si chiude L
    sw_mode.stop_l_enable = 1;
    sw_mode.stop_r_enable = 1;
    sw_mode.pol_stop_l    = 0;
    sw_mode.pol_stop_r    = 0;
    sw_mode.en_softstop   = 0;
    Steppers[1].setSwMode(sw_mode);

    Steppers[1].setMaxSteps(612);
    Steppers[1].setMicrosteps(7);
    
    //Reset: 100,600,-3000
    //GoEnd: 100,1000,640
  }

  {
  //Stepper 2,  Da Sx a Dx
    TMC5130::SwMode sw_mode = Steppers[2].getSwMode();
    sw_mode.swap_lr       = 1;  //
    sw_mode.stop_l_enable = 0;
    sw_mode.stop_r_enable = 0;
    sw_mode.pol_stop_l    = 0;
    sw_mode.pol_stop_r    = 0;
    sw_mode.en_softstop   = 0;
    Steppers[2].setSwMode(sw_mode);

    Steppers[2].setMaxSteps(-550);
    Steppers[2].setMicrosteps(7);
    //Reset: 100,300,300
    //GoEnd: 100,300,-450
  }
}

void WaitEndOfSteps(bool ShowInfo=true){
  TMC5130::SpiStatus  Status;
  do{
    if(ShowInfo){
      Status = PrintSpiStatus(Steppers[StepperInTest]);  PrintGlobalStatus(Steppers[StepperInTest]);  Serial.println(">>");
    }else
      Status = Steppers[StepperInTest].GetSpiStatus();

  }while(Status.position_reached==0);
}

void WaitHome(bool ShowInfo=true){
  TMC5130::SpiStatus  Status;
  do{
    if(ShowInfo){
      Status = PrintSpiStatus(Steppers[StepperInTest]);  PrintGlobalStatus(Steppers[StepperInTest]);  Serial.println("<<");
    }else
      Status = Steppers[StepperInTest].GetSpiStatus();
  }while(Status.status_stop_l==0 && Status.status_stop_r==0);
}

void GoEnd() {
  uint16_t A = 100;
  uint32_t V = 1000;
  int32_t Mult = 1<<(Steppers[StepperInTest].getMicrosteps()-8);
  int32_t Steps = Steppers[StepperInTest].getMaxSteps();//*Mult;
  Serial.printf("GoToEnd: %d\n", Steps);

  InitGoto(Steppers[StepperInTest], false);
  Steppers[StepperInTest].SetTrapezoidal        (A, V); //setSecondAcceleration, setFirstDeceleration, setMaxVelocity
  Steppers[StepperInTest].setTarget(Steps);   
}

void ShowWaiting(ulong TimeOut){
  ulong timeStart = millis();
  TMC5130::SpiStatus  Status;
  do{
    Status = PrintSpiStatus(Steppers[StepperInTest]);  PrintGlobalStatus(Steppers[StepperInTest]);
    int32_t  CurrPos = Steppers[StepperInTest].getPosition();
  	Serial.printf("Pos:%d Tim=%d)\n", CurrPos, millis()-timeStart);
  }while( ((millis()-timeStart)<TimeOut) && Status.position_reached==0 );
}

void GoHome(){
  mnuZeroGoto();  //

  //Vado verso Home
	InitGoto(Steppers[StepperInTest], false);
  Steppers[StepperInTest].SetTrapezoidal        (100, 600); //setSecondAcceleration, setFirstDeceleration, setMaxVelocity

  int32_t Mult = 1<<(8-Steppers[StepperInTest].getMicrosteps());
  int32_t Steps = Steppers[StepperInTest].getMaxSteps();

  Serial.printf("GoToHome: 2*%d Mult=%d\n", Steps, Mult);

  Steppers[StepperInTest].setFirstDeceleration(1000);
  Steppers[StepperInTest].setSecondDeceleration(1000);

  if(StepperInTest==2){
    Steppers[StepperInTest].SetTrapezoidal        (100, 300); //setSecondAcceleration, setFirstDeceleration, setMaxVelocity
	  Serial.printf("\n\n\n=========================nSOLO PER QUESTO\n");
  }
  int32_t Target = -Steps*2;
	Steppers[StepperInTest].setPosition(0);
	Steppers[StepperInTest].setTarget( Target ); 
	Serial.printf("MaxSteps è %d vado a %d\n", Steps, Target);
  WaitHome();   //Aspetta fine corsa

  Serial.printf("Esco da Finecorsa...\n");
  TMC5130::SpiStatus  Status;
  do{
    Target = Steppers[StepperInTest].getPosition();
    Target = Target + ((Steps>0) ? 1 : -1);
    Serial.printf("...Passetto...\n");

    Steppers[StepperInTest].setTarget( Target );
    WaitEndOfSteps(false);

    Status = PrintSpiStatus(Steppers[StepperInTest]);  PrintGlobalStatus(Steppers[StepperInTest]);  Serial.println("++");
    //Status = Steppers[StepperInTest].GetSpiStatus();
  }while(Status.status_stop_l==1 || Status.status_stop_r==1);
  Serial.printf("Uscito da Finecorsa\n");


//  mnuHardStop();
      //setRampMode(VelocityPositiveMode);  //RAMPMODE
      Steppers[StepperInTest].setSecondAcceleration(100);           //AMAX
      Steppers[StepperInTest].setMaxVelocity(0);                  //VMAX



  Steppers[StepperInTest].setPosition(0);
  Steppers[StepperInTest].setCurrent(0, 0, 0);
  Steppers[StepperInTest].setPosition(0);

  Serial.printf("A)Nuova posizione è  %d \n", Steppers[StepperInTest].getPosition());
}

long GetANumber(char* Prompt){
  Serial.println(Prompt);
  while (Serial.available() < 2) ;
  return Serial.parseInt();
}





void ChangeMotor(void){
  ClearScreen();
  Serial.print("\n\nCurrent Motor is: "); Serial.println(Steppers[StepperInTest].GetName());

  uint Size = wxSIZEOF(Steppers) + 1;
  char* Menu[Size];
  Menu[0] = "Exit";
  for(int i=0; i<Size; i++){
    Menu[i+1] = Steppers[i].GetName();
  }
  int menuChoice;
  do{
    menuChoice = ShowMenu(Menu, Size);
    if(menuChoice>0 && menuChoice<=Size){
      StepperInTest = menuChoice-1;
      Serial.print("Stepper in Test: "); Serial.println(Steppers[StepperInTest].GetName());
      return;
    }
  }while(menuChoice!=0);
}

void ChangeMicroSteps(void) {
  char* Menu[] = {"Exit", "ms_256", "ms_128", "ms_64", "ms_32", "ms_16", "ms_8", "ms_4", "ms_2", "Full Step"};
  uint Size = wxSIZEOF(Menu);
  int menuChoice;
  do{
    ClearScreen();
    uint8_t ms = Steppers[StepperInTest].getMicrosteps();
    Serial.print("Actual Microsteps: "); Serial.println(Menu[ms+1]);  
    menuChoice = ShowMenu(Menu, Size);
    if(menuChoice>0 && menuChoice<=Size){
      Steppers[StepperInTest].setMicrosteps((menuChoice-1));
      Serial.print("Microsteps: "); Serial.println(Menu[StepperInTest]);
    }
  }while(menuChoice!=0);
}

void ChangeMaxVelocity(void){
  char* Menu[] = {"Exit", "+100", "-100", "+1000", "-1000"};
  uint Size = wxSIZEOF(Menu);
  
  int menuChoice;
  do{
    int64_t CurrMaxVel = Steppers[StepperInTest].getMaxVelocity();
    Serial.print("Current Max Velocity: "); Serial.println(CurrMaxVel);
    menuChoice = ShowMenu(Menu, Size);
    if(menuChoice>0 && menuChoice<=Size){
      switch(menuChoice){
        case 0: return;
        case 1: CurrMaxVel +=  100; break;
        case 2: CurrMaxVel -=  100; break;
        case 3: CurrMaxVel += 1000; break;
        case 4: CurrMaxVel -= 1000; break;
      }
      if(CurrMaxVel<0)  CurrMaxVel = 0;
      if(CurrMaxVel>0x7FFE00)  CurrMaxVel = 0x7FFE00;  //8388096
      Steppers[StepperInTest].setMaxVelocity(CurrMaxVel); 
    }
  }while(menuChoice!=0);
}

void ChangeStandStillMode(void) {
  char* Menu[] = {"Exit", "Normal", "Free Wheeling",      "Passive Braking Ls",    "Passive Braking Hs"};
  uint Size = wxSIZEOF(Menu);
  int menuChoice;
  do{
    ClearScreen();
    menuChoice = ShowMenu(Menu, Size);
    if(menuChoice>0 && menuChoice<=Size){
      if(menuChoice==2)
        Steppers[StepperInTest].setCurrent(0, 0, 0);
      else
        Steppers[StepperInTest].setCurrent(31, 31, 15);

      Steppers[StepperInTest].writeStandstillMode( (TMC5130::StandstillMode)(menuChoice-1) );
    }
  }while(menuChoice!=0);
}

void ShowMotorForce(void){
  Serial.print("FreeWheel..: ");  Serial.println( ((Steppers[StepperInTest].getPwmconf() & 0x00300000)>>20) );  //PWMCONF
  Serial.print("StealthChop: ");  Serial.println( (Steppers[StepperInTest].getGconf().en_pwm_mode==0)       );  //GCONF
  Serial.print("IHOLD......: ");  Serial.println( Steppers[StepperInTest].ShadowRegs.Ihold_Irun.ihold       );  //IHOLD_IRUN
  Serial.print("TPOWERDOWN.: ");  Serial.println(Steppers[StepperInTest].getTPowerDown()                    );  //TPOWERDOWN
  Serial.print("IHOLD_DELAY: ");  Serial.println( Steppers[StepperInTest].ShadowRegs.Ihold_Irun.iholddelay  );  //IHOLD_IRUN
  Serial.print("PWM_SCALE..: ");  Serial.println(Steppers[StepperInTest].getPwmScale().bytes                );  //PWM_SCALE
}

void SearchTime(TMC5130 &stepper){
  for(uint8_t ms=0;ms<=8; ms++){
   stepper.setMicrosteps(ms); 
  }
}

void Test_Goto(){
  InitGoto(Steppers[StepperInTest], false);

  Serial.println("Acceleration, Speed, Steps");
  while (Serial.available() < 2) ;
  StringSplitter splitter(Serial.readString(), ',', 99);
  int itemCount = splitter.getItemCount();
  if(itemCount==3){
    uint16_t A = splitter.getItemAtIndex(0).toInt();
    uint32_t V = splitter.getItemAtIndex(1).toInt();
    int32_t  S = splitter.getItemAtIndex(2).toInt();
    Steppers[StepperInTest].SetTrapezoidal        (A, V); //setSecondAcceleration, setFirstDeceleration, setMaxVelocity
    Serial.printf("Go from %d to ...", Steppers[StepperInTest].getPosition());
    Steppers[StepperInTest].setTarget(S); 
    Steppers[StepperInTest].setRampMode(TMC5130::PositionMode);

    Serial.printf("Acc=%d, Vel=%d, Steps=%d\n\n", A, V, S );

    WaitEndOfSteps();

  }
}

void mnuHardStop        ()  {Steppers[StepperInTest].StopMotor(1000);}
void mnuSoftStop        ()  {Steppers[StepperInTest].StopMotor(1);}
void mnuSetNeutral      ()  {/*Steppers[StepperInTest].setNeutral();*/Steppers[StepperInTest].setCurrent(0, 0, 0);}
void mnuSetParking      ()  {Steppers[StepperInTest].setParking();}
void mnuSetFreeRunning  ()  {Steppers[StepperInTest].setCurrent(7, 1, 1); SetFreeRunning(Steppers[StepperInTest], 2,8);}
void mnuSetPositional   ()  {SetPositional(Steppers[StepperInTest], 2,8);}
void mnuSetDirection    ()  {Steppers[StepperInTest].setRampMode(TMC5130::VelocityNegativeMode);}
void mnuZeroGoto        ()  {Steppers[StepperInTest].StopMotor(1000);  //Stop Motor
                              Steppers[StepperInTest].setRampMode(TMC5130::PositionMode);
                              Steppers[StepperInTest].setPosition(0);
                              Steppers[StepperInTest].setTarget(-200000); 
                              Steppers[StepperInTest].setRampMode(TMC5130::PositionMode);
                              
                              //Steppers[StepperInTest].moveTo(GetANumber("Get number of steps:"));
                            }
void mnuInitDS0         ()  {TMC5130_Init_DS0(Steppers[StepperInTest]);}
void mnuInitDS1         ()  {TMC5130_Init_DS1(Steppers[StepperInTest]);}


void MainMenu(){
  sMenu MainMenu[] = {  {"Change Motor",        ChangeMotor },
                        {"Hard Stop (1000)",    mnuHardStop },
                        {"Soft Stop (1)",       mnuSoftStop },
                        {"Neutral",             mnuSetNeutral },
                        {"Parking",             mnuSetParking },
                        {"Free running",        mnuSetFreeRunning },
                        {"Positional",          mnuSetPositional },
                        {"Reverse",             mnuSetDirection },
                        {"Change MicroSteps",   ChangeMicroSteps },
                        {"Change MaxVelocity",  ChangeMaxVelocity  },
                        {"Zero and GoTo",       mnuZeroGoto  },
                        {"GoTo",                Test_Goto },
                        {"Show Registers",      ShowReadableRegistersAll },
                        {"Init DS 0",           mnuInitDS0 },
                        {"Init DS 1",           mnuInitDS1 },
                        {"Stand still Mode",    ChangeStandStillMode },
//                        {"Show Motor Force",      ShowMotorForce },
                        {"Go Home",             GoHome },
                        {"GoEnd",               GoEnd},
  };

  Serial.printf("B)Nuova posizione è  %d \n", Steppers[StepperInTest].getPosition());
  int menuChoice = ShowMenu(MainMenu, wxSIZEOF(MainMenu));
  if(menuChoice>=0)
    MainMenu[menuChoice].MenuFunc();
  else{
    Serial.println("Please choose a valid selection, "); Serial.print(menuChoice); Serial.println(" is invalid!"); 
    return;
  }
  Serial.printf("C)Nuova posizione è  %d \n", Steppers[StepperInTest].getPosition());
}

void loop(void){
  MainMenu();

//  loop_Fsa(0);
}
