#include <Arduino.h>
#include "../SysSampler.h"

void ShowBuffer(const uint8_t* data, uint16_t len){
    uint16_t nvalids = sizeof(sCommand); 
    Serial.printf("\nReceiced %d bytes, valids %d:\n", len, nvalids);

    Serial.print("        : ");
    for(int i=0; i<nvalids; i++) Serial.printf("%d  ", (i%10));
    Serial.println("");

    Serial.print("Show Hex: ");
    for(int i=0; i<nvalids; i++) Serial.print("x  ");
    Serial.println("");

    Serial.print("Show Hex: ");
    for(int i=0; i<len; i++) Serial.printf("%02X ", data[i]);
    Serial.println("");

    Serial.print("ASCII   : ");
    for(int i=0; i<len; i++) Serial.printf("%c  ", isprint(data[i])?data[i] : ' ');
    
    Serial.println("\n");

}