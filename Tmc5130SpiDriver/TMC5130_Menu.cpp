
#include <Arduino.h>
#include "TMC5130_Menu.h"
#include "TMC5130_Display.h"

int mnuGetChoice(uint8_t Size){
  Serial.print("\nYour choice:");

/* 4012
  String teststr;
  do{
    while (Serial.available() == 0) {RunFSA();}     //wait for data available
    teststr = Serial.readString().trim();
  }while(teststr.length()==0);
  int menuChoice = teststr.toInt();
*/

  while (Serial.available() < 2){ RunFSA(); }
  int menuChoice = Serial.parseInt();

  Serial.print(menuChoice);
  if(menuChoice <0 || menuChoice >= Size )
    return -1;
  return menuChoice;
}

//ToDo Make this a callback
void ClearScreen(void){
  Serial.print("\n\n\n\n\n\n\n\n\n");
/*
┌──────────┬──────────┐
│ Nome     │ Cognome  │
├──────────┼──────────┤
│ Mario    │ Rossi    │
│ Luigi    │ Verdi    │
└──────────┴──────────┘
*/

  Serial.printf("│Current Motor │ Position │Hold│Run│Dly│ms│");Serial.printf("            ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐             ┌─────┬─────┬─────┐\n");
  PrintStepperInfo(Steppers[StepperInTest]);  Serial.print("   ");
  PrintSpiStatus(Steppers[StepperInTest]);    Serial.print("   ");
  PrintGlobalStatus(Steppers[StepperInTest]);
  Serial.printf("\n└──────────────┴──────────┴────┴───┴───┴──┘            └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘             └─────┴─────┴─────┘\n\n");
  RunFSA();
}


int ShowMenu(char* Menu[], uint8_t Size){
  uint8_t Hg = 5; //Items per columns
  ClearScreen();
  for(int i=0; i<Size; i++){
    bool x=false;
    for(int Col=0; Col<=Hg; Col++){
      RunFSA();
      if(Size>(Hg*Col) && (i+Hg*Col)<Size && (i+Hg*Col)<(Hg*(Col+1))){
        Serial.printf("%2d) %-25s", i+Hg*Col, Menu[i+Hg*Col]);
        x=true;
      }
    }
    if(x) Serial.printf("\n");
  }
  int menuChoice = mnuGetChoice(Size);
  if(menuChoice >= 0){
    Serial.print(": ");
    Serial.println(Menu[menuChoice]);
  }
  return menuChoice;
}

int ShowMenu(sMenu Menu[], uint8_t Size){
  uint8_t Hg = 5; //Items per columns
  ClearScreen();
  for(int i=0; i<Size; i++){
    bool x = false;
    for(int Col=0; Col<=Hg; Col++){
      RunFSA();
      if(Size>(Hg*Col) && (i+Hg*Col)<Size && (i+Hg*Col)<(Hg*(Col+1))){
        Serial.printf("%2d) %-25s", i+Hg*Col, Menu[i+Hg*Col].Label);
        x=true;
      }
    }
    if(x) Serial.printf("\n");
  }
  int menuChoice = mnuGetChoice(Size);
  if(menuChoice >= 0){
    Serial.print(": ");
    Serial.println(Menu[menuChoice].Label);
  }
  return menuChoice;
}
