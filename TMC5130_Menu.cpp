
#include <Arduino.h>
#include "TMC5130_Menu.h"

int mnuGetChoice(uint8_t Size){
  Serial.print("\nYour choice:");
  while (Serial.available() < 2) ;
  int menuChoice = Serial.parseInt();
  Serial.print(menuChoice);
  if(menuChoice <0 || menuChoice >= Size )
    return -1;
  return menuChoice;
}

//ToDo Make this a callback
void ClearScreen(void){
  Serial.print("\n\n\n\n\n\n\n\n\n");

  TMC5130 *Mot = &Steppers[StepperInTest];
  Serial.printf("Current Motor | Position |Hold|Run|Dly|\n");
  Serial.printf("%14s|%10d|%4d|%3d|%3d|\n"
                      , Mot->GetName()
                      , Mot->getPosition()
                      , Mot->ShadowRegs.Ihold_Irun.ihold
                      , Mot->ShadowRegs.Ihold_Irun.irun
                      , Mot->ShadowRegs.Ihold_Irun.iholddelay
  );
  Serial.printf("--------------|----------|----|---|---|\n\n");
}


int ShowMenu(char* Menu[], uint8_t Size){
  uint8_t Hg = 5; //Items per columns
  ClearScreen();
  for(int i=0; i<Size; i++){
    bool x=false;
    for(int Col=0; Col<=Hg; Col++){
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
