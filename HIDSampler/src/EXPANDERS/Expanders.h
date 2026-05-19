#pragma once
#include <Arduino.h>

#include <Wire.h>
#include <TCA9555.h>          //I/O Expanders
#include "../SysSampler.h"


#define PIN_INT_MOT     48  // va al piedino 25 == IO48 dell'ESP
#define PIN_INT_IO      13  // va al piedino 21 == IO13 dell'ESP
//#define PIN_CUT_ALERT   ??  // va al piedino  ==  dell'ESP
#define PIN_NEEDLE_CRSH 1   // va al piedino 39 == IO01 dell'ESP

class cExpSampler{
	uint16_t m_LastOutput=0xFFFF;
public:
	typedef enum : uint8_t {
		eAll,eA1,eA2, eA3, eP1, eP2, eP3,
	}eExpOutputs;


	cExpSampler(void);
	~cExpSampler(void){};
	bool			WriteOut(uint16_t Mask, bool en);
	void			Exp_Setup(void);
	static uint16_t	getMask( eExpOutputs Ch);
//	uint16_t		GetLastOutput(void) {return m_LastOutput;};

	uint16_t		GetLastOutput(void) {
							uint16_t Ret = 0;
							Ret |= (m_LastOutput & getMask(eA1)) ? 0 : 0x01;
							Ret |= (m_LastOutput & getMask(eA2)) ? 0 : 0x02;
							Ret |= (m_LastOutput & getMask(eA3)) ? 0 : 0x04;
							Ret |= (m_LastOutput & getMask(eP1)) ? 0 : 0x08;
							Ret |= (m_LastOutput & getMask(eP2)) ? 0 : 0x10;
							Ret |= (m_LastOutput & getMask(eP3)) ? 0 : 0x20;

							return Ret;
					};
};


#ifdef EEEEEEEEEEEEEEEEEEEEEEEEEEEE


class SamplerInterrupts {
  public:
    volatile bool flagINT_MOT     = false;
    volatile bool flagINT_IO      = false;
    volatile bool flagNeedleCrash = false;

    // Ctor
    SamplerInterrupts(uint8_t PIN_INT_MOT, uint8_t PIN_INT_IO, uint8_t PIN_NEEDLE_CRSH) 
      : _pinMot(pinMot), _pinIo(pinIo), _pinNeedle(pinNeedle) {}

    void begin() {
      pinMode(_pinMot,    INPUT_PULLUP);
      pinMode(_pinIo,     INPUT_PULLUP);
      pinMode(_pinNeedle, INPUT_PULLUP);

      // Usiamo lambda per collegare i metodi della classe
      attachInterrupt(digitalPinToInterrupt(_pinMot),     [this]() { IRAM_ATTR handleMOT();     }, CHANGE);
      attachInterrupt(digitalPinToInterrupt(_pinIo),      [this]() { IRAM_ATTR handleIO();      }, CHANGE);
      attachInterrupt(digitalPinToInterrupt(_pinNeedle),  [this]() { IRAM_ATTR handleNeedle();  }, CHANGE);
    }

  private:
    uint8_t _pinMot, _pinIo, _pinNeedle;

    // Metodi di gestione (devono essere velocissimi)
    void IRAM_ATTR handleMOT()    { flagINT_MOT = true; }
    void IRAM_ATTR handleIO()     { flagINT_IO = true; }
    void IRAM_ATTR handleNeedle() { flagNeedleCrash = true; }
};



volatile bool flagINT_MOT      = false;  uint16_t last_INT_MOT = 0xFFFF;
volatile bool flagINT_IO       = false;  uint16_t last_INT_IO  = 0xFFFF;
volatile bool flagNeedleCrash  = false;

void IRAM_ATTR handleMOTInterrupt     (void) { flagINT_MOT      = true;}
void IRAM_ATTR handleIOInterrupt      (void) { flagINT_IO       = true;}
void IRAM_ATTR handleNeedleInterrupt  (void) { flagNeedleCrash  = true;}

void Interrupts_Setup(void){
  pinMode(PIN_INT_MOT,      INPUT_PULLUP);  attachInterrupt(digitalPinToInterrupt(PIN_INT_MOT),       handleMOTInterrupt,     CHANGE);
  pinMode(PIN_INT_IO,       INPUT_PULLUP);  attachInterrupt(digitalPinToInterrupt(PIN_INT_IO ),       handleIOInterrupt,      CHANGE);
  pinMode(PIN_NEEDLE_CRSH,  INPUT_PULLUP);  attachInterrupt(digitalPinToInterrupt(PIN_NEEDLE_CRSH ),  handleNeedleInterrupt,  CHANGE);
}



  typedef union  {    //
    struct {
      union  {    //
        struct {
          bool OUT1       : 1;  //A2 Elettrovalvola Ingresso Siringa Diluitore                  8000
          bool OUT2       : 1;  //A1 Elettrovalvola Acqua/Aria Ingresso al vaso di espansione   4000
          bool BC_FlashDL : 1;  //I 01                                                         2000
          bool BC_Cts     : 1;  //I 06                                                         1000
          bool BC_Rts     : 1;  //O 07
          bool BC_PwrDown : 1;  //O 08
          bool BC_WakeUp  : 1;  //I 11
          bool BC_Trig    : 1;  //I 12
        };
        uint8_t byte;
      },  //Exp8h_20
      union  {    //
        struct {
          bool Ext_Cs     : 1;  //
          bool OUT3       : 1;  // A1 Elettrovalvola Acqua pozzetto lavaggio ago
          bool OUT4       : 1;  // P1 Pompa Carico Acqua/Aria
          bool OUT5       : 1;  // P2 Pompa Scarico pozzetto di lavaggio  
          bool OUT6       : 1;  // P3 Aux
          bool IN_1       : 1;  // S7  Switch Galleggiante Vaso di espansione
          bool IN_2       : 1;  // S7  Switch Livello Tanica Carico
          bool IN_3       : 1;  // S7  Switch Livello Tanica Scarico
        };
        uint8_t byte;
      },  //Exp8l_20
    };
    uint16_t bytes;
  }Exp16hlF_20;


  typedef union  {    //
    struct {
      bool OUT1       : 1;  //A2 Elettrovalvola Ingresso Siringa Diluitore                  8000
      bool OUT2       : 1;  //A1 Elettrovalvola Acqua/Aria Ingresso al vaso di espansione   4000
      bool BC_FlashDL : 1;  //I 01                                                         2000
      bool BC_Cts     : 1;  //I 06                                                         1000
      bool BC_Rts     : 1;  //O 07
      bool BC_PwrDown : 1;  //O 08
      bool BC_WakeUp  : 1;  //I 11
      bool BC_Trig    : 1;  //I 12
      bool Ext_Cs     : 1;  //
      bool OUT3       : 1;  // A1 Elettrovalvola Acqua pozzetto lavaggio ago
      bool OUT4       : 1;  // P1 Pompa Carico Acqua/Aria
      bool OUT5       : 1;  // P2 Pompa Scarico pozzetto di lavaggio  
      bool OUT6       : 1;  // P3 Aux
      bool IN_1       : 1;  // S7  Switch Galleggiante Vaso di espansione
      bool IN_2       : 1;  // S7  Switch Livello Tanica Carico
      bool IN_3       : 1;  // S7  Switch Livello Tanica Scarico
     };
    uint16_t bytes;
  }Exp16_20;



//Addr: 0x20
  /*
  === 0x20 ===

*  15  OUT1         A2 Elettrovalvola Ingresso Siringa Diluitore                  8000
*  14  OUT2         A1 Elettrovalvola Acqua/Aria Ingresso al vaso di espansione   4000
-  13  BC_FlashDL    I 01                                                         2000
-  12  BC_Cts        I 06                                                         1000
*  11  BC_Rts        O 07
*  10  BC_PwrDown    O 08
-  09  BC_WakeUp     I 11
-  08  BC_Trig       I 12
*  07  Ext_Cs  
*  06  OUT3        A1 Elettrovalvola Acqua pozzetto lavaggio ago
*  05  OUT4        P1 Pompa Carico Acqua/Aria
*  04  OUT5        P2 Pompa Scarico pozzetto di lavaggio  
*  03  OUT6        P3 Aux
-  02  IN_1        S7  Switch Galleggiante Vaso di espansione
-  01  IN_2        S7  Switch Livello Tanica Carico
-  00  IN_3        S7  Switch Livello Tanica Scarico
*/

/*
  === 0x21 ===
    ┌─────────────── None20         0x8000    //
    │┌────────────── None19         0x4000    //
    ││┌───────────── M4_CE          0x2000    //
    │││┌──────────── M4_CS          0x1000    //
    ││││┌─────────── M5_CE          0x0800    //
    │││││┌────────── M5_CS          0x0400    //
    ││││││┌───────── M6_CE          0x0200    //
    │││││││┌──────── M6_CS          0x0100    //
    ││││││││┌─────── M1_CE          0x0080    //
    │││││││││┌────── M1_CS          0x0040    //
    ││││││││││┌───── M2_CS          0x0020    //
    │││││││││││┌──── M2_CE          0x0010    //
    ││││││││││││┌─── M3_CE          0x0008    //
    │││││││││││││┌── M3_CS          0x0004    //
    ││││││││││││││┌─ None5          0x0002    //
    │││││││││││││││┌ None4          0x0001    //
  0b1100000000000011
 */


 /*
  === 0x22 ===
    ┌─────────────── Unused0           0x8000    //
    │┌────────────── Unused1           0x4000    //
    ││┌───────────── M4_Diag_0         0x2000    //
    │││┌──────────── M4_Diag_1         0x1000    //
    ││││┌─────────── M5_Diag_0         0x0800    //
    │││││┌────────── M5_Diag_1         0x0400    //
    ││││││┌───────── M6_Diag_0         0x0200    //
    │││││││┌──────── M6_Diag_1         0x0100    //
    ││││││││┌─────── M1_Diag_0         0x0080    //
    │││││││││┌────── M1_Diag_1         0x0040    //
    ││││││││││┌───── M2_Diag_0         0x0020    //
    │││││││││││┌──── M2_Diag_1         0x0010    //
    ││││││││││││┌─── M3_Diag_0         0x0008    //
    │││││││││││││┌── M3_Diag_1         0x0004    //
    ││││││││││││││┌─ SampleTouch       0x0002    //
    │││││││││││││││┌ Needle Crash      0x0001    //
  0b1111111111111111
  Int_Mot
  */

#endif