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
  assert(irun>=0     && irun<=31 && 
        ihold>=0     && ihold<=31  && 
        holdDelay>=0 && holdDelay<=15);

//  uint32_t old = readReg(GCONF)|0x10000;  //SetDirectMode
//  enableDirectMode(true);

/*
  IRUN: Motor run current (0=1/32…31=32/32) Hint: Choose sense resistors in a way, that normal IRUN is 16 to 31 for best microstep performance.
  IHOLD:  Standstill current (0=1/32…31=32/32)  In combination with StealthChop mode, setting IHOLD=0 allows to choose freewheeling or coil short circuit for motor stand still.
  IHOLDDELAY: Controls the number of clock cycles for motor power down after a motion as soon as standstill is detected (stst=1) and TPOWERDOWN has expired.
              The smooth transition avoids a motor jerk upon power down.
              0: instant power down
              1..15: Delay per current reduction step in multiple of 2^18 clocks
*/
  uint32_t reg = readReg(TMC5130::IHOLD_IRUN) & (~0xF1F1F);
  reg |= (ihold) & 0x1F;
  reg |= (irun<<8) & 0x1F00;
  reg |= (holdDelay<<16) & 0xF0000;

  //reg |= ((uint32_t)holdDelay << 16) | ((uint32_t)irun << 8) | (ihold);
  writeReg(IHOLD_IRUN, reg);
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


void TMC5130::SetSwMode(SwModes m){
  assert(m>=(SwModes)0 && m<=(SwModes)0xFFF);
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

TMC5130::TMC5130(SPIClass &spiRef, uint8_t csPin, SPI_ENABLER_CB cbCS, uint32_t spiHz, char* Name): cbEnableChipSelect(cbCS), csPinAddress(csPin), StepName(Name){
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

bool TMC5130::zeroVelocity(void) {
  //return ((readReg(RAMP_STAT) & 0x200) !=0);
  RampStat ramp_stat;
  ramp_stat.bytes = readReg(RAMP_STAT); // & 0x200
  return ramp_stat.vzero;
}

void TMC5130::cacheControllerSettings(ControllerParameters &Ret) {
  //In Polidoro's routine, ther get registers from a local copy (registers_ptr_)
//  ControllerParameters Ret;
  Ret.ramp_mode             = getRampMode();  //(RampMode)registers_ptr_->getStored(Registers::RAMPMODE);
  SwMode sw_mode;  sw_mode.bytes = readReg(SW_MODE);
  Ret.stop_mode             = (StopMode)sw_mode.en_softstop;
  Ret.max_velocity          = readReg(VMAX);
  Ret.max_acceleration      = readReg(AMAX);
  Ret.start_velocity        = readReg(VSTART);
  Ret.stop_velocity         = readReg(VSTOP);
  Ret.first_velocity        = readReg(V1);
  Ret.first_acceleration    = readReg(A1);
  Ret.max_deceleration      = readReg(DMAX);
  Ret.first_deceleration    = readReg(D1);
  Ret.zero_wait_duration    = readReg(TZEROWAIT);
  Ret.stall_stop_enabled    = sw_mode.sg_stop;
  Ret.min_dc_step_velocity  = readReg(VDCMIN);
//  return Ret;
}

void TMC5130::beginRampToZeroVelocity( void ){
  // cacheControllerSettings();
  writeReg(VSTART, 0); //writeStartVelocity(0);
  writeReg(VMAX,   0); //writeMaxVelocity(0);
}


int32_t TMC5130::Init_MicroSteps(uint8_t ms){
  assert(ms>=0 && ms<=8);
  setMicrosteps(ms);

  int32_t Steps     = 200;
  uint16_t FirstAcc = 10;
  uint32_t FirstVel = 10;
  uint32_t MaxVel   = 100;
  uint32_t SpeedFor1RPS   = 53687>>ms;  //Velocity for 1 Rotation Per Second

  switch(ms){
    case  8: FirstAcc =  50;    FirstVel = 100;    MaxVel =   200; Steps =   200; break;  //210
    case  7: FirstAcc =  50;    FirstVel = 100;    MaxVel =   400; Steps =   400; break;  //419
    case  6: FirstAcc =  50;    FirstVel = 100;    MaxVel =   800; Steps =   800; break;  //839
    case  5: FirstAcc =  50;    FirstVel = 100;    MaxVel =  1600; Steps =  1600; break;  //1678
    case  4: FirstAcc =  50;    FirstVel = 100;    MaxVel =  3200; Steps =  3200; break;  //3355
    case  3: FirstAcc =  50;    FirstVel = 100;    MaxVel =  6400; Steps =  6400; break;  //6711
    case  2: FirstAcc =  50;    FirstVel = 100;    MaxVel = 12800; Steps = 12800; break;  //13422
    case  1: FirstAcc =  50;    FirstVel = 100;    MaxVel = 25600; Steps = 25600; break;  //26844
    case  0: FirstAcc =  50;    FirstVel = 100;    MaxVel = 51200; Steps = 51200; break;  //53687
    default: FirstAcc =  10;    FirstVel =  10;    MaxVel =   100; Steps =   200; break;
  }

  FirstAcc = 1000;
//  MaxVel = 100000;
  setFirstAcceleration(FirstAcc);
  setFirstVelocity(FirstVel);
  setMaxVelocity(MaxVel*2);
  return Steps;
}
