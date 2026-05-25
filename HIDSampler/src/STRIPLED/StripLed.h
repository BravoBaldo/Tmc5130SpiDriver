#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "../SysSampler.h"

/*
Task Dedicato: Poiché l'ESP32 è dual-core, potresti far girare AlwaysRun() su un Task separato (Core 0) per non rallentare il resto del codice.
Usa le funzioni di FastLED: Invece di cicli for manuali per il fade, usa fadeToBlackBy(leds, NUM_LEDS, 50);. È molto più fluido.
Luminosità: Usa FastLED.setBrightness(128); nel setup invece di manipolare i valori RGB manualmente per scurire i LED.

*/

// === STRIPLED ===
#define NUM_LEDS    22
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define DATA_PIN    14
#define VOLTS       5
#define MAX_MA      500
#define DELAYBASE   30

#define NUMSEGMENTS 7

class cStripLed{
	CRGB			leds[NUM_LEDS];
	StripGame		m_CurrGame;
	unsigned long	m_lastUpdate	= 0;
	int				m_step			= 0;	// Sostituisce i 'j' statici
	int				m_dir			= 1;	// Per rimbalzi o direzioni
	bool			m_RunAlways;
	byte			m_NumShowed		= 0;

	void resetAnimation() {
		m_step = 0;
		m_dir = 1;
		m_lastUpdate = millis();
		FastLED.clear();
	}

  void fTest_MoveSingle2(bool Dir, int n) {
    FastLED.clear();    // 1. Spegniamo tutti i LED prima di disegnare il nuovo frame

    // 2. Disegniamo i LED della "scia" (da m_step-n a m_step+n)
    for (int i = -n; i <= n; i++) {
        int targetIdx = m_step + i;

        // Verifichiamo che l'indice sia all'interno della striscia
        if (targetIdx >= 0 && targetIdx < NUM_LEDS) {
            // Se Dir è false, invertiamo l'indice di visualizzazione
            int displayIdx = (!Dir) ? (NUM_LEDS - 1 - targetIdx) : targetIdx;
            leds[displayIdx] = CRGB(255, 255, 255);//CRGB::White; 
        }
    }
    m_step++; if (m_step >= NUM_LEDS) m_step = 0;    // 3. Avanzamento del contatore di classe
  }

  void fTest_MoveFlagItaly(bool Dir){
    int Idx;
    for(int i=0; i<NUM_LEDS; i++){
      Idx = (!Dir)       ? (NUM_LEDS-1-i) : i;
      if(Dir){
        if(i==m_step)        leds[Idx] = CRGB(255,   0,   0);  //Rosso
        else if(i==m_step-1) leds[Idx] = CRGB(255, 255, 255);  //Bianco
        else if(i==m_step-2) leds[Idx] = CRGB(  0, 255,   0);  //Verde
        else            leds[Idx] = CRGB(  0,   0,   0);
      }else{
        if(i==m_step)        leds[Idx] = CRGB(  0, 255,   0);  //Rosso
        else if(i==m_step-1) leds[Idx] = CRGB(255, 255, 255);  //Bianco
        else if(i==m_step-2) leds[Idx] = CRGB(255,   0,   0);  //Verde
        else            leds[Idx] = CRGB(  0,   0,   0);
      }
    }
    m_step++; if(m_step>=NUM_LEDS) m_step=0;
  }

  void fTest_MoveArrow(bool Dir){
    int Idx;
    for(int i=0; i<NUM_LEDS; i++){
      Idx = (!Dir)       ? (NUM_LEDS-1-i) : i; 
      if(i==m_step)        leds[Idx] = CRGB(255, 255, 255);
      else if(i==m_step-1) leds[Idx] = CRGB(100, 100, 100);
      else if(i==m_step-2) leds[Idx] = CRGB(32, 32, 32);
      else            leds[Idx] = CRGB(  0,   0,   0);
    }
    m_step++; if(m_step>=NUM_LEDS) m_step=0;
  }

  void fTest_Bouncing() {
    static int Idx = 0;
    static int Inc = 1;
	
    FastLED.clear(); 
	
    // 1. Controllo di sicurezza: Idx deve restare tra 0 e NUM_LEDS-1
    if (Idx >= 0 && Idx < NUM_LEDS) leds[Idx] = CRGB(255, 255, 255);
    Idx += Inc;
    if (Idx >= NUM_LEDS - 1) { Idx = NUM_LEDS - 1; Inc = -1; }
    if (Idx <= 0) { Idx = 0; Inc = 1; }
  }

  void fTest_Fade(void) {
    fill_solid(leds, NUM_LEDS, CHSV(0, 0, m_step)); // CHSV(H, S, V)
    m_step += m_dir;
    if (m_step >= 255 || m_step <= 0) m_dir *= -1;
  }

  void fTest_Italy(bool Lr){
    int i=0;
    for(;i< NUM_LEDS/3;  i++)   leds[i] = Lr?CRGB(  0, 255,   0) : CRGB(255,   0,   0);   //(Lr ? CRGB::Green : CRGB::Red); // CRGB(  0, 255,   0);  //Red
    for(;i<(NUM_LEDS/3)*2;i++)  leds[i] = CRGB(255, 255, 255);                            //CRGB::White;              //  //White
    for(;i< NUM_LEDS;     i++)  leds[i] = Lr?CRGB(255,   0,   0) : CRGB(  0, 255,   0);   //(Lr ? CRGB::Red : CRGB::Green); //CRGB(255,   0,   0);  //Green
  }

  void fTest_Grays(bool Dir){
    for(int i=0; i<NUM_LEDS; i++){
      int v = (255/NUM_LEDS)*i;
      if(Dir)
        leds[i] = CRGB(v, v, v);
      else
        leds[NUM_LEDS-1-i] = CRGB(v, v, v);
    }
  }

  void HighlightSegment(int N, int X, CRGB colorSel=CRGB(10, 0, 0), CRGB colorEven=CRGB(2, 2, 4), CRGB colorOdd=CRGB(2, 4, 2)) {
    if (N <= 0 || X < 0 || X >= N) {
      FastLED.clear();
      FastLED.show();
      return;
    }
  
    int ledAcc = 0;
    for (int s = 0; s < N; s++) {
      int nextLed = ((s + 1) * NUM_LEDS) / N;
      CRGB currColor = (s == X) ? colorSel : ((s & 1) ? colorOdd : colorEven);
      for (int l = ledAcc; l < nextLed; l++)   leds[l] = currColor;
      ledAcc = nextLed;  // Il punto di inizio del prossimo segmento è la fine di questo
    }  
    FastLED.show();
  }

public:
  cStripLed() : m_CurrGame(eNone) {}
  StripGame getCurrGame(void)	{return m_CurrGame;};

  void Init() {
      // Rimosso FastLED.delay(3000) che blocca l'esecuzione dell'ESP32
      FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
      FastLED.setMaxPowerInVoltsAndMilliamps(VOLTS, MAX_MA);
      FastLED.clear(true);
      FastLED.show();
      m_RunAlways = false;
  }
  
  unsigned long m_Timer = 0;
  unsigned long m_TimerT = 0;
  
  bool ResetTimer(void){
	  m_Timer = 0;
	  return true;
  }
  
  bool SetTimer(unsigned long Timer){
	  if(m_Timer!=0)	return false;
	  m_Timer = Timer;
	  m_TimerT = millis();
	  return true;
  }

  bool WaitTimer(void){
	  return (m_Timer==0);
  }
  
  unsigned long Remaining(void){
	if(m_Timer==0)	return 0;
    return ( m_Timer-(millis() - m_TimerT)  );
  }

  bool SetGame(StripGame Game){
    m_CurrGame = Game;
    resetAnimation();

    //Only the first time
    m_RunAlways = true; 
    switch(m_CurrGame){
      case eFixedItalianFlagL:  fTest_Italy(true );  m_RunAlways = false; break;
      case eFixedItalianFlagR:  fTest_Italy(false);  m_RunAlways = false; break;
      case eGraysSx:            fTest_Grays(true );  m_RunAlways = false; break;
      case eGraysDx:            fTest_Grays(false);  m_RunAlways = false; break;
      default:                  return true;
    }
    FastLED.show();
	return true;
  }

  void AlwaysRun(void){
	if(m_Timer!=0){
		if( (millis() - m_TimerT) >= m_Timer )
			m_Timer = 0;
	}
	  
    if(m_RunAlways==false || (millis() - m_lastUpdate) < DELAYBASE) {
      yield(); 
      return;
    }
    switch(m_CurrGame){
      case eMoveSingle_Sx:      fTest_MoveSingle2(true, 0); break;
      case eMoveSingle_Dx:      fTest_MoveSingle2(false,0); break;
      case eMoveDouble_Sx:      fTest_MoveSingle2(true,5);  break;
      case eMoveDouble_Dx:      fTest_MoveSingle2(false,5); break;
      case eMoveFlagItaly_Sx:   fTest_MoveFlagItaly(true);  break;
      case eMoveFlagItaly_Dx:   fTest_MoveFlagItaly(false); break;
      case eMoveArrow_Sx:       fTest_MoveArrow(true);      break;
      case eMoveArrow_Dx:       fTest_MoveArrow(false);     break;
      case eFade:               fTest_Fade();               break;
      case eBouncing:           fTest_Bouncing();           break;
	  case eShowNum:			HighlightSegment(NUMSEGMENTS, m_NumShowed);			break;
      //default:                  return;
    }
    FastLED.show();
    yield(); 
    m_lastUpdate = millis();
  }
  
  void setNumShowed(byte s){m_NumShowed=s;}

};
