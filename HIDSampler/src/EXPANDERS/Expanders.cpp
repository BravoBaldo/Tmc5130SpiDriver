#include "Expanders.h"

// === I2C ===
#define I2C_SDA   38
#define I2C_SCL   39 
#define I2C_FREQ  4000000   //4000000

extern TCA9555   Expanders[];

cExpSampler::cExpSampler(void){
	//Expanders[0].write16(0xFFFF);
	//Serial.println("RESET EXPANDER 0!");
};

uint16_t cExpSampler::getMask( eExpOutputs Ch){
	switch(Ch){
		case eA1:	return 0x8000;
		case eA2:	return 0x4000;
		case eA3:	return 0x0040;
		case eP1:	return 0x0020;
		case eP2:	return 0x0010;
		case eP3:	return 0x0008;
		case eAll:
		default:	return 0x8000 | 0x4000 | 0x0040 | 0x0020 | 0x0010 | 0x0008;
	}
	
}

bool cExpSampler::WriteOut(uint16_t Mask, bool en){
	//uint16_t Old = Expanders[0].read16();
    m_LastOutput = en ? (m_LastOutput & ~Mask) : (m_LastOutput | Mask);
    Expanders[0].write16(m_LastOutput);	
	return true;
}



void cExpSampler::Exp_Setup(void){
  Serial.print("Initializing I2C...");
  if( !Wire.begin(I2C_SDA, I2C_SCL) ){
    Wire.setClock(I2C_FREQ);
    Serial.println("Error: I2C not found. Check your wiring!");
    while (1);
  }
  Serial.println("OK");
  //----------------------------------------------------------
  Serial.print("Initializing I2C @ 0x20, 0x21, 0x22: Expanders...");
  Expanders[0].pinMode16(0b0000110000000111); //0x20
  Expanders[1].pinMode16(0b1100000000000011); //0x21
  Expanders[2].pinMode16(0xFFFF);             //0x22
  
  m_LastOutput = getMask(eAll);
  WriteOut(m_LastOutput, false);
  //Expanders[0].write16(m_LastOutput);
  Serial.println("OK");
  //----------------------------------------------------------  
  
}
