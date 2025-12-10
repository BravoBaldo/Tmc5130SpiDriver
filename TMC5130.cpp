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
  SPI_Status.bytes = spi->transfer( Read ? ((uint8_t)reg & 0x7F) : ((uint8_t)reg | 0x80) );
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

void TMC5130::setCurrent(uint8_t irun, uint8_t ihold, uint8_t holdDelay) {  //IHOLD_IRUN
  assert(irun>=0     && irun<=31 && 
        ihold>=0     && ihold<=31  && 
        holdDelay>=0 && holdDelay<=15);

  //IHOLD_IRUN is WriteOnly
  ShadowRegs.Ihold_Irun.bytes = 0;
  ShadowRegs.Ihold_Irun.irun       = irun;
  ShadowRegs.Ihold_Irun.ihold      = ihold;
  ShadowRegs.Ihold_Irun.iholddelay = holdDelay;
  writeReg(IHOLD_IRUN, ShadowRegs.Ihold_Irun.bytes);
}

void TMC5130::setNodeConf(uint8_t naddr, uint8_t sdelay) { //SLAVECONF or NODECONF WriteOnly
  assert(naddr>=0     && naddr<=253 && 
        sdelay>=0 && sdelay<=15);
  ShadowRegs.NODECONF.bytes = 0;
  ShadowRegs.NODECONF.nodeaddr  = naddr;
  ShadowRegs.NODECONF.senddelay = sdelay;
  writeReg(NODECONF, ShadowRegs.NODECONF.bytes);
}

void TMC5130::setXCompare(int32_t xcomp) { //X_COMPARE WriteOnly
  ShadowRegs.X_COMPARE = xcomp;
  writeReg(NODECONF, ShadowRegs.X_COMPARE);
}

void TMC5130::setTHigh(uint32_t v){
  assert(v>=0 && v<=0xFFFFF);
  writeReg(IHOLD_IRUN, v);
  ShadowRegs.THIGH = v;
}

void TMC5130::Sethold_delay(uint8_t hold_delay){
  assert(hold_delay>=0 && hold_delay<=15);
  //IHOLD_IRUN is write Only
  ShadowRegs.Ihold_Irun.iholddelay = hold_delay;
  writeReg(IHOLD_IRUN, ShadowRegs.Ihold_Irun.bytes);
}

void TMC5130::SetIhold(uint8_t ihold){
  assert(ihold>=0 && ihold<=31);
  //IHOLD_IRUN is write Only
  ShadowRegs.Ihold_Irun.ihold = ihold;
  writeReg(IHOLD_IRUN, ShadowRegs.Ihold_Irun.bytes);
}

void TMC5130::SetIrun(uint8_t irun){
  assert(irun>=0 && irun<=31);
  //IHOLD_IRUN is write Only
  ShadowRegs.Ihold_Irun.irun = irun;
  writeReg(IHOLD_IRUN, ShadowRegs.Ihold_Irun.bytes);
}

void TMC5130::setMicrosteps(uint8_t mres) {
  assert(mres>=0 && mres<=8);
  uint32_t v = readReg(CHOPCONF) & 0xF0FFFFFF;
  uint32_t mr = (((uint32_t)mres)<<24);
  v |= mr;
  writeReg(CHOPCONF, v);
}

void TMC5130::softwareEnable(){  //CHOPCONF
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.toff = 3;  //from 1...15 enabled_toff_;  //Off time setting controls duration of slow decay phase
  writeReg(TMC5130::CHOPCONF, chopconf.bytes);
}

void TMC5130::writeChopperMode(ChopperMode chopper_mode) {  //CHOPCONF
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.chm = chopper_mode;
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::enableHighVelocityFullstep(bool En) { //CHOPCONF //True=enable, False=disable
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.vhighfs = En?1:0;
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::enableHighVelocityChopperSwitch(bool En) {  //CHOPCONF //True=enable, False=disable
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.vhighchm = En?1:0;
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::enableShortToGroundProtection(bool En) {  //CHOPCONF  //True=enable=0, False=disable=1
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.diss2g = En?0:1; //Note: Inverted
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::writeComparatorBlankTime(ComparatorBlankTime tbl) {  //CHOPCONF
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.tbl = tbl;
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::writeEnabledToff(uint8_t toff) { //CHOPCONF
//  %0000: Driver disable, all bridges off
//  %0001: 1 – use only with TBL ≥ 2

//  const static uint8_t DISABLE_TOFF         = 0b0;
//  const static uint8_t ENABLED_TOFF_MIN     = 1;
//  const static uint8_t ENABLED_TOFF_MAX     = 15;
//  const static uint8_t ENABLED_TOFF_DEFAULT = 3;

  if (toff <  1)  toff =  1;
  if (toff > 15)  toff = 15;
  Chopconf chopconf;
  chopconf.bytes = readReg(CHOPCONF);
  chopconf.toff = toff;
  writeReg(CHOPCONF, chopconf.bytes);
}

void TMC5130::setGconf(uint32_t gconf){
  assert(gconf>=0 && gconf <= 0x1FFFF);
  writeReg(GCONF, gconf);
}

void TMC5130::setMotorDirection(MotorDirection motor_direction) {  //GCONF  True=1=ReverseDirection, false=0=ForwardDirection
    Gconf gconf = getGconf();
    gconf.shaft = motor_direction;
    setGconf(gconf.bytes);
}

void TMC5130::setDcStep(uint16_t dcTime, uint16_t dcSG) {
  assert(dcTime>=0 && dcTime<=0x1FF  &&  dcSG>=0 && dcSG<=0xFF);
  uint32_t v = ((uint32_t)dcTime << 16) | dcSG;
  setDcctrl(v);
}

void TMC5130::setTPwmThrs(uint32_t TPwmThrs){
  assert(TPwmThrs>=0 && TPwmThrs<=0xFFFFF);
  writeReg(TCOOLTHRS, TPwmThrs);
  ShadowRegs.TPwmThrs = TPwmThrs;
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

void TMC5130::setMaxVelocity(uint32_t v) {
  assert(v>=0 && v<=0x7FFE00);
  writeReg(VMAX, v);
  ShadowRegs.VMAX = v;
}

void TMC5130::setStartVelocity(uint32_t v){
  assert(v>=0 && v<=0x3FFFF);
  writeReg(VSTART, v);
}

void TMC5130::setFirstVelocity(uint32_t v)  {
  assert(v>=0 && v<=0xFFFFF);
  writeReg(V1, v);
  ShadowRegs.V1 = v;
}

void TMC5130::setStopVelocity(uint32_t v)  {
  assert(v>=0 && v<=0x3FFFF);
  writeReg(VSTOP, v);
  ShadowRegs.VSTOP = v;
}

void TMC5130::setFirstAcceleration(uint16_t a) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(a>=0 && a<=0xFFFF);
  writeReg(A1, a);
  ShadowRegs.A1 = a;
}

void TMC5130::setSecondAcceleration (uint16_t a) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(a>=0 && a<=0xFFFFF);
  writeReg(AMAX, a);
  ShadowRegs.AMAX = a;
}

void TMC5130::setFirstDeceleration(uint16_t d) {  //[μsteps / ta²]  0...65535=0xFFFF
  assert(d>=0 && d<=0xFFFF);
  writeReg(DMAX, d);
  ShadowRegs.DMAX = d;
}

void TMC5130::setSecondDeceleration(uint16_t d) { //[μsteps / ta²]  0...65535=0xFFFF
  assert(d>=1 && d<=0xFFFF);
  writeReg(D1, d);
  ShadowRegs.D1 = d;
}

void TMC5130::setRampMode(RampMode m) {
  assert(m>=0 && m<=3);
  writeReg(RAMPMODE, m);
}


bool TMC5130::zeroVelocity(void) {
  //return ((readReg(RAMP_STAT) & 0x0400) !=0);
  RampStat ramp_stat;
  ramp_stat.bytes = readReg(RAMP_STAT); //
  return ramp_stat.vzero;
}

bool TMC5130::positionReached(void) {  //RAMP_STAT
  RampStat ramp_stat;
  ramp_stat.bytes = readReg(RAMP_STAT); //0x0200
  return ramp_stat.position_reached;  //bit 9
}

uint16_t TMC5130::readStallGuardResult(void) { //DRV_STATUS
  DrvStatus drv_status;
  drv_status.bytes = readReg(DRV_STATUS);
  return drv_status.sg_result;
}

void TMC5130::writeStopMode(StopMode stop_mode) { //SW_MODE
  SwMode sw_mode;
  sw_mode.bytes = readReg(SW_MODE);
  sw_mode.en_softstop = stop_mode;
  writeReg(SW_MODE, sw_mode.bytes);
}

void TMC5130::enableStallStop(bool En) { //SW_MODE //True=enable, False=disable
  SwMode sw_mode;
  sw_mode.bytes = readReg(TMC5130::SW_MODE);
  sw_mode.sg_stop = En?1:0;
  writeReg(TMC5130::SW_MODE, sw_mode.bytes);
}

void TMC5130::cacheControllerSettings(ControllerParameters &Ret) {
  //In Polidoro's routine, ther get registers from a local copy (registers_ptr_)
  Ret.ramp_mode             = getRampMode();                  //VelocityPositiveMode
  SwMode sw_mode;  sw_mode.bytes = readReg(SW_MODE);
  Ret.stop_mode             = (StopMode)sw_mode.en_softstop;  //HardMode
  Ret.max_velocity          = getMaxVelocity();                  //10
  Ret.max_acceleration      = getSecondAcceleration();        //AMAX)//10  WriteOnly
  Ret.start_velocity        = readReg(VSTART);                //1
  Ret.stop_velocity         = getStopVelocity();            //10  VSTOP ReadOnly
  Ret.first_velocity        = readReg(V1);                    //0
  Ret.first_acceleration    = readReg(A1);                    //0
  Ret.max_deceleration      = getFirstDeceleration();         //1   DMAX WriteOnly
  Ret.first_deceleration    = getSecondDeceleration();        //10  D1 WriteOnly
//R Ret.zero_wait_duration    = readReg(TZEROWAIT);             //0
  Ret.stall_stop_enabled    = sw_mode.sg_stop;                //false
//R  Ret.min_dc_step_velocity  = readReg(VDCMIN);                //0
}

void TMC5130::writeControllerParameters(ControllerParameters &par){
  setRampMode(par.ramp_mode); //RAMPMODE
  writeStopMode         (par.stop_mode);
  setMaxVelocity        (par.max_velocity);
  setSecondAcceleration (par.max_acceleration);  //AMAX,      
  writeReg(VSTART,    par.start_velocity);
  setStopVelocity(par.stop_velocity); //VSTOP
  writeReg(V1,        par.first_velocity);
  writeReg(A1,        par.first_acceleration);
  setFirstDeceleration  (par.max_deceleration);
  setSecondDeceleration (par.first_deceleration); //D1
  writeReg(TZEROWAIT, par.zero_wait_duration);
  enableStallStop(par.stall_stop_enabled);
  writeReg(VDCMIN,    par.min_dc_step_velocity);
}

void TMC5130::beginRampToZeroVelocity( void ){
  // cacheControllerSettings();
  writeReg(VSTART, 0); //writeStartVelocity(0);
  setMaxVelocity(0);
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

uint8_t TMC5130::stepAndDirectionMode(void) { //IOIN
  Ioin ioin;
  ioin.bytes = readReg(IOIN);
  return ioin.sd_mode;
}

bool TMC5130::homed() {  //ToDo: leggere le note
  // reading ramp_stat clears flags and may cause motion after stall stop
  // better to read actual velocity instead
  int32_t actual_velocity = getVelocity();  //controller.readActualVelocity();
  bool still = (actual_velocity == 0);
  if (still)
    setRampMode(HoldMode); //RAMPMODE controller.writeRampMode(HoldMode);
  return still;
}
/*
uint16_t TMC5130::readStallGuardResult() {
  Registers::DrvStatus drv_status;
  drv_status.bytes = registers_ptr_->read(Registers::DrvStatusAddress);
  return drv_status.sg_result;
}
*/

void TMC5130::endHome(void) {
  setRampMode(HoldMode);       //controller.writeRampMode(HoldMode);
  writeReg(XACTUAL,  0);                //controller.zeroActualPosition();
  setTarget(0);                //XTARGET   controller.zeroTargetPosition();

  //ToDo: driver.restoreDriverSettings();
  //ToDo: controller.restoreControllerSettings();
  //ToDo: controller.restoreSwitchSettings();

  readReg(RAMP_STAT);                 //registers.read(Registers::RampStatAddress);  // clear ramp_stat flags
  setRampMode(PositionMode); //controller.writeRampMode(PositionMode);
}

void TMC5130::writeStandstillMode(StandstillMode mode) { //PWMCONF
  Pwmconf pwmconf;
  pwmconf.bytes = getPwmconf();
  pwmconf.freewheel = mode;
  setPwmconf(pwmconf.bytes);
}

void TMC5130::writePwmOffset(uint8_t pwm_amplitude) {  //PWMCONF
  Pwmconf pwmconf;
  pwmconf.bytes = getPwmconf();
  pwmconf.pwm_ampl = pwm_amplitude;    //pwm_ampl alias pwm_ofs
  setPwmconf(pwmconf.bytes);
}

void TMC5130::writePwmGradient(uint8_t pwm_amplitude) {  //PWMCONF
  TMC5130::Pwmconf pwmconf;
  pwmconf.bytes = getPwmconf();
  pwmconf.pwm_grad = pwm_amplitude;
  setPwmconf(pwmconf.bytes);
}

void TMC5130::enableAutomaticCurrentControl(bool en) {  //PWMCONF
  TMC5130::Pwmconf pwmconf;
  pwmconf.bytes = getPwmconf();
  pwmconf.pwm_autoscale = en?1:0;
  pwmconf.pwm_symmetric = en?1:0; //alias pwm_autograd
//pwmconf.pwm_reg = pwm_reg;      //pwm_reg does not exists in 5130
  setPwmconf(pwmconf.bytes);
}
