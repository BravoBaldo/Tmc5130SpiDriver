/**
 * TMC5130.cpp - Implementazione super-commentata della libreria TMC5130A
 *
 * Modalità supportate:
 *  - SPI (classica, 5 byte frame con doppia transazione per lettura)
 *  - UART single-wire (protocollo Trinamic con indirizzi slave e CRC8-ATM)
 *  - STEP/DIR (uso come driver “classico”, step generati dal microcontrollore)
 *
 * Nota importante:
 *  - La modalità UART e SPI si selezionano dal pin SW_SEL del TMC5130A.
 *  - In modalità UART, i driver possono essere messi in cascata e ricevono
 *    indirizzi automaticamente tramite i pin NAI/NAO.
 */

#define NDEBUG
#include <assert.h>

#include "TMC5130.h"

//Default routine for Chip Selection
void  EnableSpiOnChip(uint8_t csPin, bool en)     { digitalWrite(csPin, en?LOW:HIGH);  }

// =====================================================
// ======= SEZIONE SPI (Serial Peripheral Interface) ====
// =====================================================

uint32_t TMC5130::genSpiFunct(Reg reg, uint32_t value, bool Read){
  spi->beginTransaction(SPISettings(spiFreq, MSBFIRST, SPI_MODE3));
  cbEnableChipSelect(csPinAddress, true);
  SPI_Status = spi->transfer( Read ? ((uint8_t)reg & 0x7F) : ((uint8_t)reg | 0x80) );
  // Successivi 4 byte: dati MSB → LSB
  uint8_t b3 = spi->transfer((value >> 24) & 0xFF);
  uint8_t b2 = spi->transfer((value >> 16) & 0xFF);
  uint8_t b1 = spi->transfer((value >>  8) & 0xFF);
  uint8_t b0 = spi->transfer((value      ) & 0xFF);
  cbEnableChipSelect(csPinAddress, false);
  spi->endTransaction();

  uint32_t val = ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) |
                 ((uint32_t)b1 << 8)  | ((uint32_t)b0);
  return val;
}

// Funzione privata: scrive un registro via SPI
void TMC5130::spiWrite(Reg reg, uint32_t value) {
  genSpiFunct(reg, value, false);
}

// Funzione privata: legge un registro via SPI
uint32_t TMC5130::spiRead(Reg reg) {
  // 1) Prima transazione = prefetch (dati non validi)
  genSpiFunct(reg, 0, true);
  // 2) Seconda transazione = dati validi
  return genSpiFunct(reg, 0, true);
}

// Inizializza interfaccia SPI
bool TMC5130::beginSPI(SPIClass &spiRef, uint32_t spiHz) {
  mode = MODE_SPI;
  spi = &spiRef;
  spiFreq = spiHz;
  
  spi->begin();
  spi->setBitOrder(SPI_MSBFIRST);
  spi->setDataMode(SPI_MODE0);

  // Reset dei flag di errore scrivendo 1 su GSTAT
  writeReg(GSTAT, 0x07);

  return true;
}

// =====================================================
// ======= SEZIONE UART (Single-wire / RS485-like) =====
// =====================================================

// ---- CRC8-ATM (polinomio x^8 + x^2 + x^1 + 1, valore 0x07) ----
static uint8_t calcCRC8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }
  return crc;
}

// Inizializza UART
bool TMC5130::beginUART(HardwareSerial &serial, uint8_t slaveAddr, long baud) {
  mode = MODE_UART;
  uart = &serial;
  uartAddr = slaveAddr;
  uartBaud = baud;

  uart->begin(baud);

  return true;
}

// Invio pacchetto UART
void TMC5130::sendUART(uint8_t addr, bool write, Reg reg, uint32_t data) {
  uint8_t buf[8];

  buf[0] = 0x05; // SYNC
  buf[1] = addr & 0xFF; // indirizzo slave
  buf[2] = ((write ? 0x80 : 0x00) | ((uint8_t)reg & 0x7F)); // CMD
  buf[3] = (data >> 24) & 0xFF;
  buf[4] = (data >> 16) & 0xFF;
  buf[5] = (data >>  8) & 0xFF;
  buf[6] = (data      ) & 0xFF;
  buf[7] = calcCRC8(buf, 7); // CRC su SYNC..DATA

  for (int i = 0; i < 8; i++) uart->write(buf[i]);
}

// Ricezione pacchetto UART (semplificata)
bool TMC5130::recvUART(uint8_t addr, Reg reg, uint32_t &data) {
  uint8_t buf[8];
  int i = 0;
  unsigned long t0 = millis();
  while (i < 8 && (millis() - t0) < 50) { // timeout 50ms
    if (uart->available()) {
      buf[i++] = uart->read();
    }
  }
  if (i < 8) return false; // timeout

  // Verifica CRC
  uint8_t crc = calcCRC8(buf, 7);
  if (crc != buf[7]) return false;

  // Estrai dati
  data = ((uint32_t)buf[3] << 24) |
         ((uint32_t)buf[4] << 16) |
         ((uint32_t)buf[5] <<  8) |
         ((uint32_t)buf[6]);
  return true;
}

// Scrive un registro via UART
void TMC5130::writeRegUART(uint8_t slave, Reg reg, uint32_t value) {
  sendUART(slave, true, reg, value);
}

// Legge un registro via UART
uint32_t TMC5130::readRegUART(uint8_t slave, Reg reg) {
  sendUART(slave, false, reg, 0);
  uint32_t val = 0;
  if (recvUART(slave, reg, val)) return val;
  else return 0xFFFFFFFF; // errore
}

// =====================================================
// ======= SEZIONE STEP/DIR ============================
// =====================================================

void TMC5130::beginStepDir(uint8_t stepPin, uint8_t dirPin, uint8_t enPin) {
  mode = MODE_STEPDIR;
  pinSTEP = stepPin;
  pinDIR  = dirPin;
  pinEN   = enPin;

  pinMode(pinSTEP, OUTPUT);
  pinMode(pinDIR, OUTPUT);
  if (pinEN != 0xFF) {
    pinMode(pinEN, OUTPUT);
    digitalWrite(pinEN, LOW); // tipicamente EN è attivo basso
  }
  digitalWrite(pinSTEP, LOW);
  digitalWrite(pinDIR, LOW);
}

void TMC5130::stepOnce() {
  if (pinSTEP == 0xFF) return;
  digitalWrite(pinSTEP, HIGH);
  delayMicroseconds(2);
  digitalWrite(pinSTEP, LOW);
  delayMicroseconds(2);
}

void TMC5130::setDirection(bool dirCW) {
  if (pinDIR == 0xFF) return;
  digitalWrite(pinDIR, dirCW ? HIGH : LOW);
}

void TMC5130::enableDriver(bool en) {
  if (pinEN == 0xFF) return;
  digitalWrite(pinEN, en ? LOW : HIGH); // attivo basso
}

// =====================================================
// ======= CONFIGURAZIONI COMUNI =======================
// =====================================================

void TMC5130::writeReg(Reg reg, uint32_t value) {
  if (mode == MODE_SPI) spiWrite(reg, value);
  else if (mode == MODE_UART) writeRegUART(uartAddr, reg, value);
}

uint32_t TMC5130::readReg(Reg reg) {
  if (mode == MODE_SPI) return spiRead(reg);
  else if (mode == MODE_UART) return readRegUART(uartAddr, reg);
  else return 0;
}

void TMC5130::enableRegBit(bool en, Reg reg, uint32_t Mask) {
  uint32_t old = readReg(reg);
  if(en) old |= Mask; else old &= ~Mask;
  writeReg(reg, old);
}

void TMC5130::setCurrent(uint8_t irun, uint8_t ihold, uint8_t holdDelay) {
  assert(irun>=0 && irun<=31 && ihold>=0 && ihold<=31  && holdDelay>=0 && holdDelay<=15);

  uint32_t old = readReg(GCONF)|0x10000;  //SetDirectMode
//  enableDirectMode(true);

/*
  IRUN: Motor run current (0=1/32…31=32/32) Hint: Choose sense resistors in a way, that normal IRUN is 16 to 31 for best microstep performance.
  IHOLD:  Standstill current (0=1/32…31=32/32)  In combination with StealthChop mode, setting IHOLD=0 allows to choose freewheeling or coil short circuit for motor stand still.
  IHOLDDELAY: Controls the number of clock cycles for motor power down after a motion as soon as standstill is detected (stst=1) and TPOWERDOWN has expired.
              The smooth transition avoids a motor jerk upon power down.
              0: instant power down
              1..15: Delay per current reduction step in multiple of 2^18 clocks
*/
  uint32_t v = ((uint32_t)holdDelay << 16) | ((uint32_t)irun << 8) | (ihold);
  writeReg(IHOLD_IRUN, v);
}

void TMC5130::setMicrosteps(uint8_t mres) {
  assert(mres>=1 && mres<=0b1000);
  uint32_t v = readReg(CHOPCONF) & 0xF0FFFFFF;
  uint32_t mr = (((uint32_t)mres)<<24);
  v |= mr;
  writeReg(CHOPCONF, v);
}

void TMC5130::setMicrosteps(MicroStepsMask m) {
  uint32_t v = readReg(CHOPCONF);
  v = (v & 0xF0FFFFFF) | m;
  writeReg(CHOPCONF, v);
}

void TMC5130::SetGconfBit(GconfBits b, bool en){
  uint32_t gconf = readReg(GCONF);
  if (en) gconf |= b;
  else    gconf &= ~b;
  writeReg(GCONF, gconf);
}


void TMC5130::SetSwMode(int32_t m){
  assert(m>=0 && m<=0xFFF);
  writeReg(SW_MODE, m );
}

void TMC5130::setDcStep(uint16_t dcTime, uint16_t dcSG) {
  assert(dcTime>=0 && dcTime<=0x1FF  &&  dcSG>=0 && dcSG<=0xFF);
  
  uint32_t v = ((uint32_t)dcTime << 16) | dcSG;
  writeReg(DCCTRL, v);
}

void TMC5130::setTCOOLTHRS(uint32_t val) {
  assert(val>=0 && val<=1048575);
  writeReg(TCOOLTHRS, val);
}

TMC5130::TMC5130(SPIClass &spiRef, uint8_t csPin, SPI_ENABLER_CB cbCS, uint32_t spiHz): cbEnableChipSelect(cbCS), csPinAddress(csPin){
  beginSPI(spiRef, spiHz);  
}

// ====================================================
// ======= MOTION CONTROLLER EMBEDDED =================
// ====================================================

void TMC5130::setRampMode(TMC5130_RampMode m) {
  assert(m>=0 && m<=3);
  writeReg(RAMPMODE, m);
}

void TMC5130::setMaxVelocity(uint32_t v) {
  assert(v>=0 && v<=0x7FFE00);
  writeReg(VMAX, v);
}

void TMC5130::setStartVelocity(uint32_t v){
  assert(v>=0 && v<=0x3FFFF);
  writeReg(VSTART, v);
}

void TMC5130::setFirstVelocity  (uint32_t v)  {
  assert(v>=0 && v<=0xFFFFF);
  writeReg(V1, v);
}

void TMC5130::setStopVelocity   (uint32_t v)  {
  assert(v>=0 && v<=0x3FFFF);
  writeReg(VSTOP, v);
}

void TMC5130::setFirstAcceleration(uint16_t a) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(a>=0 && a<=0xFFFF);
  writeReg(A1, a);   
}

void TMC5130::setSecondAcceleration (uint16_t a) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(a>=0 && a<=0xFFFFF);
  writeReg(AMAX, a);
}

void TMC5130::setFirstDeceleration(uint16_t d) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(d>=0 && d<=0xFFFF);
  writeReg(DMAX, d);
}

void TMC5130::setSecondDeceleration(uint16_t d) { //[μsteps / ta²]  0...65535=0xFFFF
  assert(d>=1 && d<=0xFFFF);
  writeReg(D1, d);
}

void TMC5130::Init_00(void){
  writeReg(TMC5130::GCONF,       0);
  writeReg(TMC5130::GSTAT,       0);
  writeReg(TMC5130::IFCNT,       0);
  writeReg(TMC5130::SLAVECONF,   0);
  writeReg(TMC5130::IOIN,        0x1100002F); //TMC5130_IOIN_OUTPUT
  writeReg(TMC5130::X_COMPARE,   0x00000000);
  writeReg(TMC5130::IHOLD_IRUN,  0x00070101);  //
  
  writeReg(TMC5130::TPOWERDOWN,  0x00000000);
  writeReg(TMC5130::TSTEP,       0x0000004D);
  writeReg(TMC5130::TPWMTHRS,    0x00000000);
  writeReg(TMC5130::TCOOLTHRS,   0x00000000);
  writeReg(TMC5130::THIGH,       0x00000000);
  writeReg(TMC5130::RAMPMODE,    0x00000002);
  writeReg(TMC5130::XACTUAL,     0xFFE38782);
  writeReg(TMC5130::VACTUAL,     0x00FCB924);
  writeReg(TMC5130::VSTART,      0x00000000);
  writeReg(TMC5130::A1,          0x000003C6);
  writeReg(TMC5130::V1,          0x0001A36E);

  writeReg(TMC5130::AMAX,        0x000003C6);
  writeReg(TMC5130::VMAX,        0x000346DC);
  writeReg(TMC5130::DMAX,        0x000003C6);
  writeReg(TMC5130::D1,          0x000003C6);
  writeReg(TMC5130::VSTOP,       0x0000000A);
  writeReg(TMC5130::TZEROWAIT,   0x00000000);
  writeReg(TMC5130::XTARGET,     0x00002710);
  writeReg(TMC5130::VDCMIN,      0x00000000);
  writeReg(TMC5130::SW_MODE,     0x00000000);
  writeReg(TMC5130::RAMP_STAT,   0x00000103);
  writeReg(TMC5130::XLATCH,      0x00000000);
  writeReg(TMC5130::ENCMODE,     0x00000000);
  writeReg(TMC5130::X_ENC,       0x00000000);
  writeReg(TMC5130::ENC_CONST,   0x00010000);
  writeReg(TMC5130::ENC_STATUS,  0x00000000);
  writeReg(TMC5130::ENC_LATCH,   0x00000000);

  //reset default microstep table:
  writeReg(TMC5130::MSLUT_0,     0xAAAAB554);  //0xAAAAB554
  writeReg(TMC5130::MSLUT_1,     0x4A9554AA);  //0x4A9554AA
  writeReg(TMC5130::MSLUT_2,     0x24492929);  //0x24492929
  writeReg(TMC5130::MSLUT_3,     0x10104222);  //0x10104222
  writeReg(TMC5130::MSLUT_4,     0xFBFFFFFF);  //0xFBFFFFFF
  writeReg(TMC5130::MSLUT_5,     0xB5BB777D);  //0xB5BB777D
  writeReg(TMC5130::MSLUT_6,     0x49295556);  //0x49295556
  writeReg(TMC5130::MSLUT_7,     0x00404222);  //0x00404222
  writeReg(TMC5130::MSLUTSEL,    0xFFFF8056);  //0xFFFF8056  X1=128, X2=255, X3=255  W3=%01, W2=%01, W1=%01, W0=%10
  writeReg(TMC5130::MSLUTSTART,  0x00F70000);  //0x00F70000  START_SIN_0= 0, START_SIN90= 247

  writeReg(TMC5130::MSCNT,       0x0000006B);
  writeReg(TMC5130::MSCURACT,    0x00F601E5);
  writeReg(TMC5130::CHOPCONF,    0x000101D5);  //**** 00010000000111010101
  writeReg(TMC5130::COOLCONF,    0x00000000);
  writeReg(TMC5130::DCCTRL,      0x00000000);
  writeReg(TMC5130::DRV_STATUS,  0x61010000);
  writeReg(TMC5130::PWMCONF,     0x000500C8);
  writeReg(TMC5130::PWMSTATUS,   0x00000000); //TMC5130_PWM_SCALE
  writeReg(TMC5130::ENCM_CTRL,   0x00000000);
  writeReg(TMC5130::LOST_STEPS,  0x00000000); 
}

void TMC5130::Init_01(void){
                                  //00010000000011000011
  setChopconf          (65731);  //writeReg(CHOPCONF,    0x000100C3);  //SPI send: 0xEC000100C3; // CHOPCONF: TOFF=3, HSTRT=4, HEND=1, TBL=2, CHM=0 (SpreadCycle)

//  irun      = 0x1F; //31  Corrente a motore in moto 0...31
//  ihold     = 0xA;  //10  Corrente a motore fermo 0...31
//  holdDelay = 0x06; //6   0: spegnimento istantaneo, 1..15: Ritardo per ogni step di riduzione della corrente in multipli di 2^18 clock
  setCurrent(31, 10, 6);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6

  writeReg(TPOWERDOWN,  0x0000000A);  //SPI send: 0x910000000A; // TPOWERDOWN=10: Delay before power down in stand still
  writeReg(GCONF,       0x00000004);  //SPI send: 0x8000000004; // EN_PWM_MODE=1 enables StealthChop (with default PWMCONF)
  writeReg(TPWMTHRS,    0x000001F4);  //SPI send: 0x93000001F4; // TPWM_THRS=500 yields a switching velocity about 35000 = ca. 30RPM
  writeReg(PWMCONF,     0x000401C8);  //SPI send: 0xF0000401C8;

  setFirstAcceleration  (1000);                   //writeReg(A1,          0x000003E8);  //SPI send: 0xA4000003E8; // A1 = 1 000 First acceleration
  setFirstVelocity      (50000);                  //writeReg(V1,          0x0000C350);  //SPI send: 0xA50000C350; // V1 = 50 000 Acceleration threshold velocity V1
  setSecondAcceleration (500);                    //writeReg(AMAX,        0x000001F4);  //SPI send: 0xA6000001F4; // AMAX = 500 Acceleration above V1
  setMaxVelocity        (200000);                 //writeReg(VMAX,        0x00030D40);  //SPI send: 0xA700030D40; // VMAX = 200 000
  setFirstDeceleration  (700);                    //writeReg(DMAX,        0x000002BC);  //SPI send: 0xA8000002BC; // DMAX = 700 Deceleration above V1
  setSecondDeceleration (1400);                   //writeReg(D1,          0x00000578);  //SPI send: 0xAA00000578; // D1 = 1400 Deceleration below V1
  setStopVelocity       (10);                     //writeReg(VSTOP,       0x0000000A);  //SPI send: 0xAB0000000A; // VSTOP = 10 Stop velocity (Near to zero)
  setRampMode           (TMC5130_MODE_POSITION);  //writeReg(RAMPMODE,    0x00000000);  //SPI send: 0xA000000000; // RAMPMODE = 0 (Target position move)

#define ENABLE_STALLGUARD
//  StopEnable(true);
  SetSwMode(0
    | SW_MODE_STOP_L_ENABLE | SW_MODE_POL_STOP_L  //Enable Left Limit Switch
  //  | SW_MODE_STOP_R_ENABLE
  //  | SW_MODE_POL_STOP_R
  //  | SW_MODE_SWAP_LR
#if defined(ENABLE_STALLGUARD)
      | SW_MODE_SG_STOP     //Enable StallGuard2
#endif
  );
  setCurrent(1, 1, 1);  //  writeReg(IHOLD_IRUN,  0x00061F0A);  //SPI send: 0x9000061F0A; // IHOLD_IRUN: IHOLD=10, IRUN=31 (max. current), IHOLDDELAY=6
  setMicrosteps(8);
#if defined(ENABLE_STALLGUARD)
  setTCOOLTHRS(100);
#endif

}

int32_t TMC5130::Init_MicroSteps(uint8_t ms){
  assert(ms>=0 && ms<=8);
  setMicrosteps(ms);

  int32_t Steps     = 200;
  uint16_t FirstAcc = 10;
  uint32_t FirstVel = 10;
  uint32_t MaxVel   = 100;
  switch(ms){
    case  8: FirstAcc =  50;    FirstVel = 100;    MaxVel =   200; Steps =   200; break;
    case  7: FirstAcc =  50;    FirstVel = 100;    MaxVel =   400; Steps =   400; break;
    case  6: FirstAcc =  50;    FirstVel = 100;    MaxVel =   800; Steps =   800; break;
    case  5: FirstAcc =  50;    FirstVel = 100;    MaxVel =  1600; Steps =  1600; break;
    case  4: FirstAcc =  50;    FirstVel = 100;    MaxVel =  3200; Steps =  3200; break;
    case  3: FirstAcc =  50;    FirstVel = 100;    MaxVel =  6400; Steps =  6400; break;
    case  2: FirstAcc =  50;    FirstVel = 100;    MaxVel = 12800; Steps = 12800; break;
    case  1: FirstAcc =  50;    FirstVel = 100;    MaxVel = 25600; Steps = 25600; break;
    case  0: FirstAcc =  50;    FirstVel = 100;    MaxVel = 51200; Steps = 51200; break;
    default: FirstAcc =  10;    FirstVel =  10;    MaxVel =   100; Steps =   200; break;
  }

  FirstAcc = 1000;
//  MaxVel = 100000;
  setFirstAcceleration(FirstAcc);
  setFirstVelocity(FirstVel);
  setMaxVelocity(MaxVel*2);
  return Steps;
}
