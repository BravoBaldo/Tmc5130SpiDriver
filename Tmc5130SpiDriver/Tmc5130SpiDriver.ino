/*
ToDo:
  Fsa's for movements
    1) una sola FSA per volta da 'attaccare' ad uno stepper al momento e da chiamare continuamente
    2) Ogni Stepper ha una sua FSA che viene chiamata continuamente
    Movimenti possibili
      Stop(Hard, Soft, variable)
      Stay(Parking, Neutral)
      Change MicroStep
      GoTo
      Reset
      GoHome
  GoTo and wait target
  Eecute an "HomeToStall"
  Check if to use Polidoro's class printer....
  Implementare gli algoritmi descritti nel datasheet
  Inserire i valori di default per ciascun motore con la possibilità di re-inizializzarlo
Tests
  Seriale
  connessione
  Fermata 'morbida':  Ok StopMotor
  In 'Neutral=Folle'  Ok
  In 'Parking'        Ok
  MotorA continuous rotation, MotB two laps forward and two back
  GoTo... steps

Done:
  https://github.com/khoih-prog/ESP32TimerInterrupt
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

//#define TEST_TIMER

#if defined(TEST_TIMER)
  hw_timer_t *timer = NULL;
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void ARDUINO_ISR_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);    // Increment the counter and set the time of ISR
      RunFSA();
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif

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

struct Fsa {  //Finite State Machine for each Motors
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

  //ShowReadableRegistersAll();
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
    Steppers[0].setMaxSteps(-3500);
    Steppers[0].setMicrosteps(8); //Full Step
    Steppers[0].setGoHomeVelocity(400);
    //Reset: 100,600,3000     100,600,-1
    //GoEnd: 100,2000,-38o0
    //Prosegue verso numeri negativi

    Steppers[0].setResets(); //Read current settings and use them as defaults
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
    Steppers[1].setGoHomeVelocity(400);
    
    //Reset: 100,600,-3000
    //GoEnd: 100,1000,640

    Steppers[1].setResets(); //Read current settings and use them as defaults
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
    Steppers[2].setGoHomeVelocity(200);
    //Reset: 100,300,300
    //GoEnd: 100,300,-450

    Steppers[2].setResets(); //Read current settings and use them as defaults
  }

#if defined(TEST_TIMER)
  timer = timerBegin(1000000);    // Set timer frequency to 1Mhz
  timerAttachInterrupt(timer, &onTimer);    // Attach onTimer function to our timer.
  timerAlarm(timer, 100000, true, 0);     //void TIMER_IRAM timerAlarm(hw_timer_t *timer, uint64_t alarm_value, bool autoreload, uint64_t reload_count)
#endif

}

void RunResetAllTimers(void){
  for(int i=0; i<wxSIZEOF(Steppers); i++)
    Steppers[i].setFsaMaxTime();
}

void RunFSA(void){
  for(int i=0; i<wxSIZEOF(Steppers); i++){
    Steppers[i].FSA_loop();
  }
}

void GoEnd() {
  while(Steppers[StepperInTest].Exec_GoEnd()==false)  RunFSA();
}

void GoEndAll() {
  //AAA Insert a TimeOut
  uint16_t Executed = (1<<wxSIZEOF(Steppers))-1;  //Put as many ones as there are steppers
  while(Executed !=0){
    for(int i=0; i<wxSIZEOF(Steppers); i++){
      if( (Executed & (1<<i))!= 0 ){              //Check the i-th stepper
        if(Steppers[i].Exec_GoEnd())
          Executed &= (~((1<<i)));                //Reset the i-th stepper
      }
      RunFSA();
    }
  }
}

void GoHome(){
  Steppers[StepperInTest].Exec_GoHome();
}

void GoHomeAll(){
  for(int i=0; i<wxSIZEOF(Steppers); i++)
    Steppers[i].Exec_GoHome();
}

void GoTo(uint16_t A, uint32_t V, int32_t  S){
    Steppers[StepperInTest].Exec_GoTo(A,V,S);
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
  char* Menu[] = {"ms_256", "ms_128", "ms_64", "ms_32", "ms_16", "ms_8", "ms_4", "ms_2", "Full Step"};
  ClearScreen();
  uint8_t ms = Steppers[StepperInTest].getMicrosteps();
  Serial.print("Actual Microsteps: "); Serial.println(Menu[ms]);  
  uint Size = wxSIZEOF(Menu);
  int menuChoice = ShowMenu(Menu, Size);
  if(menuChoice<Size){
    Steppers[StepperInTest].setMicrosteps((menuChoice));
    Serial.print("Microsteps: "); Serial.println(Menu[StepperInTest]);
  }
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

void mnuHardStop        ()  {Steppers[StepperInTest].Exec_Stop(1000);}
void mnuPanicAll        ()  {for(int i=0; i<wxSIZEOF(Steppers); i++) Steppers[i].Exec_Panic(5000);}

void mnuSoftStop        ()  {Steppers[StepperInTest].Exec_Stop(1);}
void mnuSetNeutral      ()  {/*Steppers[StepperInTest].setNeutral();*/Steppers[StepperInTest].setCurrent(0, 0, 0);}
void mnuSetNeutralAll   ()  { for(int i=0; i<wxSIZEOF(Steppers); i++) {
                                /*Steppers[StepperInTest].setNeutral();*/
                                Steppers[i].setCurrent(0, 0, 0);
                              }
                            }

void mnuSetParking      ()  {Steppers[StepperInTest].setParking();}
void mnuSetParkingAll   ()  { for(int i=0; i<wxSIZEOF(Steppers); i++) Steppers[StepperInTest].setParking(); }

void mnuSetFreeRunning  ()  {Steppers[StepperInTest].setCurrent(7, 1, 1); SetFreeRunning(Steppers[StepperInTest], 2,8);}
void mnuSetPositional   ()  {SetPositional(Steppers[StepperInTest], 2,8);}
void mnuSetDirection    ()  {Steppers[StepperInTest].setRampMode(TMC5130::VelocityNegativeMode);}
void mnuZeroGoto        ()  { Steppers[StepperInTest].Exec_StopAndZero(); }
void mnuInitDS0         ()  {TMC5130_Init_DS0(Steppers[StepperInTest]);}
void mnuInitDS1         ()  {TMC5130_Init_DS1(Steppers[StepperInTest]);}

void Demo(){
  ulong DelayDemo = millis();    //Timer
  while(( (millis()-DelayDemo) < 60000)){
    for(int i=0; i<wxSIZEOF(Steppers); i++){
      if(Steppers[i].IsFSAFree()){
        if(abs(Steppers[i].getPosition()-Steppers[i].getMaxSteps())<50)
          Steppers[i].Exec_GoHome();
        else
          Steppers[i].Exec_GoEnd();
      }
      RunFSA();
    }
  }
}

void Test_Goto(){
  Serial.println("Acceleration, Speed, Steps");
  while (Serial.available() < 2) RunFSA();
  StringSplitter splitter(Serial.readString(), ',', 99);
  int itemCount = splitter.getItemCount();
  if(itemCount==3){
    uint16_t A = splitter.getItemAtIndex(0).toInt();
    uint32_t V = splitter.getItemAtIndex(1).toInt();
    int32_t  S = splitter.getItemAtIndex(2).toInt();
    GoTo(A,V,S);
  }
}

bool Sampler_ParseCommand(const char* strCmd) {
  if(strCmd==nullptr || strlen(strCmd)==0)  return false;
  typedef enum : uint8_t {eGoTo, eHome, eLast} eCommands;

  //               Motor, Accel, Veloc, Steps, Command 
  typedef enum : uint8_t {pMotor, pAccel, pVelocity, pSteps, pCommand, pLast} eParams;
  bool ParamsF[eParams::pLast] = {false, false, false, false, false};
  long ParamsV[eParams::pLast];

  StringSplitter splitter(strCmd, ';', 99);
  int itemCount = splitter.getItemCount();
  int Motor = -1;
  int Accel = -1;
  int Veloc = -1;
  int GoTo  = 99;
  Serial.printf("Trovati %d parametri:\n", itemCount);
  for(int i=0; i<itemCount; i++){
    String Cmd = splitter.getItemAtIndex(i);
    if(Cmd.length()==0) continue;
    char C = Cmd.charAt(0);
    Serial.printf("%d) %-5s (%d) %c: ", i, Cmd, Cmd.length(), C);
    int Num = (Cmd.length()>1) ? Cmd.substring(1).toInt() : -1;
    switch(C){
      case 'm': //Motor
        if(Num>=0 && Num <=wxSIZEOF(Steppers)){
           Motor = Num;  
          ParamsF[eParams::pMotor] = true;  ParamsV[eParams::pMotor] = Num;
          Serial.printf("Motor %d",         Num);
        }
      break;
      case 'a':
        if(Num>0){
          ParamsF[eParams::pAccel] = true;  ParamsV[eParams::pAccel] = Num;
          Serial.printf("Acceleration %d",  Num);
          Accel = Num;
      }
      break;
      case 'v':
        if(Num>0){
          ParamsF[eParams::pVelocity] = true;  ParamsV[eParams::pVelocity] = Num;
          Veloc = Num;  
          Serial.printf("Velocity %d",      Num);
        }
      break;
      case 'G':
        ParamsF[eParams::pSteps]   = true;  ParamsV[eParams::pSteps  ] = Num;
        ParamsF[eParams::pCommand] = true;  ParamsV[eParams::pCommand] = eCommands::eGoTo;
        Serial.printf("GoTo %d",          Num);
        GoTo = Num;  
      break;
      case 'H':
        ParamsF[eParams::pCommand] = true;  ParamsV[eParams::pCommand] = eCommands::eHome;
        Serial.printf("GoHome %d",        Num);
        break;
//      case 'M': Serial.printf("MicroStep %d",     Num); break;
    }
    Serial.printf("\n");

    Serial.printf("Execute GoTo %d:%d,%d,%d\n", Motor, Accel,Veloc,GoTo);
    if(Motor>=0 && Accel>0 && Veloc>0 && GoTo!=99){
      do{
        RunFSA();
      }while( Steppers[Motor].Exec_GoTo(Accel,Veloc,GoTo)==false );
    }
  }
  Serial.printf("---------------------\n");
  return true;
}

void TestCommands(void){
  Sampler_ParseCommand("m1;a100;v2000;G400");
  Sampler_ParseCommand("m2;a100;v2000;G-1000");
  Sampler_ParseCommand("m0;a100;v2000;G-1000");
  Sampler_ParseCommand("m2;a100;v2000;G-10");
  Sampler_ParseCommand("m1;a100;v2000;G200");

  Sampler_ParseCommand("m0;a100;v2000;G0");
  Sampler_ParseCommand("m1;a100;v2000;G0");
  Sampler_ParseCommand("m2;a100;v2000;G0");

  Sampler_ParseCommand("m1;a100;v2000;G500");
  Sampler_ParseCommand("m2;a100;v2000;G-2000");
  Sampler_ParseCommand("m0;a100;v2000;G-2000");
  Sampler_ParseCommand("m2;a100;v2000;G-100");
  Sampler_ParseCommand("m1;a100;v2000;G300");
}


void TestParseCommand(void){
  while (Serial.available() != 0){ RunFSA(); Serial.read();}  //Flush Input
  Serial.println("\nEnter Command:");
  while (Serial.available() < 2){ RunFSA(); }
  String Cmd = Serial.readStringUntil('\n');  // Read until newline
  if(Cmd.length()==0) return;
  Serial.printf("Letto '%s'\n", Cmd.c_str());
  Sampler_ParseCommand(Cmd.c_str());

}



struct ParsedParams{
  const char* Description;
  long  Min;
  long  Max;
  long  Value;
  bool  IsSet = false;
};

bool Sampler_ParseCommandNew(const char* strCmd) {
  if(strCmd==nullptr || strlen(strCmd)==0)  return false;
  typedef enum : uint8_t {eNone, eGoTo, eHome, eReset, eNeutral, eCurrent, eMicroStep, eGoEnd} eCommands;

  StringSplitter splitter(strCmd, ';', 99);
  int itemCount = splitter.getItemCount();
  int Motor = -1;
  int Accel = -1;
  int Veloc = -1;
  int32_t Steps = 0;
//  int GoTo  = 99;
  eCommands Command = eNone;
  Serial.printf("Trovati %d parametri:\n", itemCount);
  for(int i=0; i<itemCount; i++){
    String Cmd = splitter.getItemAtIndex(i);
    if(Cmd.length()==0) continue;
    char C = Cmd.charAt(0);
    Serial.printf("%d) %-5s (%d) %c: ", i, Cmd, Cmd.length(), C);
    int Num = (Cmd.length()>1) ? Cmd.substring(1).toInt() : -1;
    switch(C){
      case 'm': //Motor
        if(Num>=0 && Num <=wxSIZEOF(Steppers)){
           Motor = Num;  
          Serial.printf("Motor %d",         Num);
        }
      break;
      case 'a':
        if(Num>0){
          Serial.printf("Acceleration %d",  Num);
          Accel = Num;
        }
      break;
      case 'v':
        if(Num>0){
          Veloc = Num;
          Serial.printf("Velocity %d",        Num);
        }
      break;
      case 'G': Command = eGoTo;        Steps = Num;  break;
      case 'H': Command = eHome;                      break;
      case 'E': Command = eGoEnd;                     break;
      case 'N': Command = eNeutral;                   break;
      case 'C': Command = eCurrent;
      //si aspetta 3 numeri separati da virgola
      break;
      case 'R': Command = eReset;                     break;
      case 's': Command = eMicroStep;  Steps = Num;   break;
    }
    Serial.printf("\n");

    switch(Command){
      case eGoTo:
        Serial.printf("Execute GoTo %d:%d,%d,%d\n", Motor, Accel,Veloc,Steps);
        if(Motor>=0 && Accel>0 && Veloc>0 && Steps!=99){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_GoTo(Accel,Veloc,Steps)==false );
        }
        break;
      case eHome:
        Serial.printf("Execute Home\n");
        if(Motor>=0){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_GoHome()==false );
        }
        break;
      case eMicroStep:
        Serial.printf("Execute eMicroStep to %d\n", Steps);
        if(Motor>=0 ){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_MicroSteps(Steps)==false );          
        }
        break;
      case eGoEnd:
        if(Motor>=0 ){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_GoEnd()==false );          
        }
        break;
      case eNeutral:
        if(Motor>=0 ){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_setCurrent(0,0,0)==false );          
        }
        break;
      case eReset:
        if(Motor>=0 ){
          do{
            RunFSA();
          }while( Steppers[Motor].Exec_WaitOperations()==false );
          Steppers[Motor].Reset();
        }
        break;
    }
  }
  Serial.printf("---------------------\n");
  return true;
}

void TestParserNew(void){
  Sampler_ParseCommandNew("m2;H");                //Home
  Sampler_ParseCommandNew("m1;a100;v2000;G400");
  Sampler_ParseCommandNew("m0;a100;v2000;G-2000");
  Sampler_ParseCommandNew("m1;H");                //Home
  Sampler_ParseCommandNew("m1;a100;v2000;G400");  //GoTo
  Sampler_ParseCommandNew("m1;s4");               //MicroStep
  Sampler_ParseCommandNew("m1;a100;v2000;G4000");  //GoTo

  Sampler_ParseCommandNew("m0;H");                //Home
  Sampler_ParseCommandNew("m1;R");                //Reset
  Sampler_ParseCommandNew("m1;H");                //Home

  Sampler_ParseCommandNew("m2;E");                //GoEnd
  Sampler_ParseCommandNew("m2;H");                //Home

  Sampler_ParseCommandNew("m0;N");                //Neutral
  Sampler_ParseCommandNew("m1;N");                //Neutral
  Sampler_ParseCommandNew("m2;N");                //Neutral
  //Set Current Ca,b,c
}

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
                        {"All GoHome",          GoHomeAll},
                        {"All GoEnd",           GoEndAll},
                        {"All Neutral",         mnuSetNeutralAll },
                        {"All Parking",         mnuSetParkingAll },
                        {"All PANIC",           mnuPanicAll },
                        {"All ResetTimers",     RunResetAllTimers},
                        {"Demo",                Demo},
                        {"TestCommands",        TestCommands},
                        {"TestParseCommand",    TestParseCommand},
                        {"TestParser New",      TestParserNew},
  };

  int menuChoice = ShowMenu(MainMenu, wxSIZEOF(MainMenu));
  if(menuChoice>=0)
    MainMenu[menuChoice].MenuFunc();
  else{
    Serial.println("\n\tPlease choose a valid selection, "); Serial.print(menuChoice); Serial.println(" is invalid!"); 
    return;
  }
}

void loop(void){
  MainMenu();
}
