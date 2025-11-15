/**
 * TMC5130.h - Libreria Arduino super-commentata per Trinamic TMC5130A
 *
 * Supporta le 3 modalità documentate dal datasheet:
 *   1. SPI (registri via interfaccia seriale sincrona)
 *   2. UART single-wire (protocollo Trinamic con CRC8-ATM, indirizzamento fino a 255 dispositivi)
 *   3. STEP/DIR (driver classico, con step generati dal microcontrollore)
 *
 * NOTA:
 *  - Il pin SW_SEL del chip sceglie la modalità (SPI o UART).
 *  - STEP/DIR può essere usato insieme a SPI/UART (per configurazione + movimento via pin).
 */

#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include <functional>  // for std::function

extern void  EnableChip(bool en);

class TMC5130 {
public:
  // ====== Modalità operative ======
  enum InterfaceMode {
    MODE_SPI,
    MODE_UART,    //NOT TESTED
    MODE_STEPDIR  //NOT TESTED
  };

  typedef enum  : uint8_t {
    TMC5130_MODE_POSITION  = 0,  //Positioning mode (using all A, D and V parameters)
    TMC5130_MODE_VELPOS    = 1,  //Velocity mode to positive VMAX (using AMAX acceleration)
    TMC5130_MODE_VELNEG    = 2,  //Velocity mode to negative VMAX (using AMAX acceleration)
    TMC5130_MODE_HOLD      = 3,  //Hold mode (velocity remains unchanged, unless stop event occurs)
  }TMC5130_RampMode;

  //MicroSteps Mask
  typedef enum  : uint32_t {
    ms_256 = 0x00000000,
    ms_128 = 0x01000000,
    ms_64  = 0x02000000,
    ms_32  = 0x03000000,
    ms_16  = 0x04000000,
    ms_8   = 0x05000000,
    ms_4   = 0x06000000,
    ms_2   = 0x07000000,
    ms_FS  = 0x08000000,
  }MicroStepsMask;

  // ====== Registri principali (estratto dal datasheet) ======
  enum Reg : uint8_t {
    GCONF       = 0x00,   // Global config
    GSTAT       = 0x01,   // Global status (write 1 to clear flags)
    IFCNT       = 0x02,   // Interface counter
    SLAVECONF   = 0x03,   // Slave config (UART)
    IOIN        = 0x04,   // Input pins
    X_COMPARE   = 0x05,   //

    IHOLD_IRUN  = 0x10,   // Corrente run/hold
    TPOWERDOWN  = 0x11,
    TSTEP       = 0x12,
    TPWMTHRS    = 0x13,
    TCOOLTHRS   = 0x14,
    THIGH       = 0x15,
   
    RAMPMODE    = 0x20,
    XACTUAL     = 0x21,
    VACTUAL     = 0x22,
    VSTART      = 0x23,
    A1          = 0x24,
    V1          = 0x25,
    AMAX        = 0x26,
    VMAX        = 0x27,
    DMAX        = 0x28,

    D1          = 0x2A,
    VSTOP       = 0x2B,
    TZEROWAIT   = 0x2C,
    XTARGET     = 0x2D,

    VDCMIN      = 0x33, //Ramp Generator Driver Feature Control Register Set
    SW_MODE     = 0x34,
    RAMP_STAT   = 0x35,
    XLATCH      = 0x36,
    ENCMODE     = 0x38,  //Encoder Registers 
    X_ENC       = 0x39,
    ENC_CONST   = 0x3A,
    ENC_STATUS  = 0x3B,
    ENC_LATCH   = 0x3C,
    SG_RESULT   = 0x40,
    MSLUT_0     = 0x60, //Motor Driver Registers  - MICROSTEPPING CONTROL REGISTER SET
    MSLUT_1     = 0x61,
    MSLUT_2     = 0x62,
    MSLUT_3     = 0x63,
    MSLUT_4     = 0x64,
    MSLUT_5     = 0x65,
    MSLUT_6     = 0x66,
    MSLUT_7     = 0x67,
    MSLUTSEL    = 0x68,
    MSLUTSTART  = 0x69,
    MSCNT       = 0x6A,
    MSCURACT    = 0x6B,
    CHOPCONF    = 0x6C,  //DRIVER REGISTER SET    
    COOLCONF    = 0x6D,
    DCCTRL      = 0x6E,
    DRV_STATUS  = 0x6F,

    PWMCONF     = 0x70,
    PWMSTATUS   = 0x71,
    PWM_SCALE   = 0x71,
    ENCM_CTRL   = 0x72,
    LOST_STEPS  = 0x73,
  };

  typedef enum  : uint32_t {
    GCONF_I_scale_analog		    = 0b000000000000000001,
    GCONF_internal_Rsense		    = 0b000000000000000010,
    GCONF_en_pwm_mode			      = 0b000000000000000100,
    GCONF_enc_commutation		    = 0b000000000000001000,
    GCONF_shaft					        = 0b000000000000010000,
    GCONF_diag0_error			      = 0b000000000000100000,
    GCONF_diag0_otpw			      = 0b000000000001000000,
    GCONF_diag0_stall			      = 0b000000000010000000,
    GCONF_diag1_stall			      = 0b000000000100000000,
    GCONF_diag1_index			      = 0b000000001000000000,
    GCONF_diag1_onstate		      =	0b000000100000000000,
    GCONF_diag1_steps_skipped 	= 0b000000100000000000,
    GCONF_diag0_int_pushpull	  = 0b000001000000000000,
    GCONF_diag1_poscomp_pushpull= 0b000010000000000000,
    GCONF_small_hysteresis		  = 0b000100000000000000,
    GCONF_stop_enable			      = 0b001000000000000000,
    GCONF_direct_mode			      = 0b010000000000000000,
    GCONF_test_mode				      = 0b100000000000000000,
  }GconfBits;

#define SW_MODE_STOP_L_ENABLE     0x001
#define SW_MODE_STOP_R_ENABLE     0x002
#define SW_MODE_POL_STOP_L        0x004
#define SW_MODE_POL_STOP_R        0x008
#define SW_MODE_SWAP_LR           0x010
#define SW_MODE_LATCH_L_ACTIVE    0x020
#define SW_MODE_LATCH_L_INACTIVE  0x040
#define SW_MODE_LATCH_R_ACTIVE    0x080
#define SW_MODE_LATCH_R_INACTIVE  0x100
#define SW_MODE_EN_LATCH_ENCODER  0x200
#define SW_MODE_SG_STOP           0x400
#define SW_MODE_EN_SOFTSTOP       0x800

  // ====== Ctors ======
#if defined(USEESPPINS)
  TMC5130(SPIClass &spiRef, uint8_t csPin,                  uint32_t spiHz = 1000000);
#endif
  TMC5130(SPIClass &spiRef, std::function<void(bool)> cbCS, uint32_t spiHz = 1000000);

  // ====== Inits ======
  bool beginSPI     (SPIClass &spiRef = SPI, uint32_t spiHz = 1000000);
  bool beginUART    (HardwareSerial &serial, uint8_t slaveAddr = 0, long baud = 115200);
  void beginStepDir (uint8_t stepPin, uint8_t dirPin, uint8_t enPin = 0xFF);

  // ====== Registry Access ======
  void writeReg   (Reg reg, uint32_t value);  // wrapper (SPI o UART)
  uint32_t readReg(Reg reg);                  // wrapper (SPI o UART)

  void writeRegUART(uint8_t slave, Reg reg, uint32_t value);  // UART diretto
  uint32_t readRegUART(uint8_t slave, Reg reg);               // UART diretto

  // ====== High level Configs ======
  void setCurrent       (uint8_t irun, uint8_t ihold, uint8_t holdDelay);
  void setMicrosteps    (uint8_t mres);
  void setMicrosteps    (MicroStepsMask m);

  inline uint8_t getMicrosteps (void)  {return (readReg(CHOPCONF)>>24) & 0x0F;}

  void SetGconfBit              (GconfBits b, bool en);
  inline void enableStealthChop (bool en)           { SetGconfBit(GCONF_diag0_stall, en); }
  inline void StopEnable        (bool en)           { SetGconfBit(GCONF_stop_enable, en); }

  inline void setChopconf       (uint32_t chopconf) { writeReg(CHOPCONF, chopconf); }
  inline void setPwmconf        (uint32_t pwmconf)  { writeReg(PWMCONF, pwmconf); }
  inline void setCoolconf       (uint32_t coolconf) { writeReg(COOLCONF, coolconf); }

  void setDcStep        (uint16_t dcTime, uint16_t dcSG);
  void setTCOOLTHRS     (uint32_t val);
  inline uint16_t readStallGuardResult  (void)    { return (uint16_t)(readReg(SG_RESULT) & 0x3FF); }
  inline void enableInverseDirection    (bool en) { enableRegBit(en, GCONF, 0x00010); }
  inline void enableStopEnable          (bool en) { enableRegBit(en, GCONF, 0x08000); }
  inline void enableDirectMode          (bool en) { enableRegBit(en, GCONF, 0x10000); }

  // Motion controller
  void setRampMode        (TMC5130_RampMode m);

  void setMaxVelocity    (uint32_t v);  //0...8388096=7FFE00
  void setStartVelocity  (uint32_t v);  //0...262143=3FFFF  //Motor start velocity
  void setFirstVelocity  (uint32_t v);  //0...1048575=FFFFF
  void setStopVelocity   (uint32_t v);  //1...262143=3FFFF  //Motor stop velocity (unsigned)

  inline int32_t getVelocity    (void)            {return ((int32_t)readReg(VACTUAL  )<<8)>>8;}

  void setFirstAcceleration   (uint16_t amax);  //[μsteps / ta²]  0...1048575=0xFFFFF First acceleration between VSTART and V1 (unsigned)
  void setSecondAcceleration  (uint16_t amax);  //[μsteps / ta²]  0...1048575=0xFFFFF Second acceleration between V1 and VMAX (unsigned)
  void setFirstDeceleration   (uint16_t dmax);  //[μsteps / ta²]  0...1048575=0xFFFFF Deceleration between VMAX and V1 (unsigned)
  void setSecondDeceleration  (uint16_t dmax);  //[μsteps / ta²]  0...1048575=0xFFFFF Deceleration between V1 and VSTOP (unsigned)

  inline void    setPosition    (int32_t x)       { writeReg(XACTUAL, x); }
  inline int32_t getPosition    (void)            { return (int32_t)readReg(XACTUAL); }

  inline void moveTo            (int32_t xTarget) { writeReg(XTARGET, (uint32_t)xTarget); setRampMode(TMC5130_MODE_POSITION); }
  inline void moveBy            (int32_t dx)      { moveTo(getPosition() + dx); }

  inline void hardStop          (void)            { setRampMode(TMC5130_MODE_HOLD); writeReg(VMAX, 0); }
  inline uint8_t getIcVersion   (void)            { return (readReg(IOIN)>>24) & 0xFF;}



  // ====== Utilities Step/Dir ======
  void stepOnce           ();
  void setDirection       (bool dirCW);
  void enableDriver       (bool en);
  void SetSwMode          (int32_t m);

  void      Init_00         (void);
  void      Init_01         (void);
  int32_t   Init_MicroSteps (uint8_t ms);
  uint8_t   GetSpiStatus    (void) {return SPI_Status;}
  uint32_t  genSpiFunct     (Reg reg, uint32_t value, bool Read);

  void      SetPinCSFunct   (std::function<void(bool)> callback)  {cbEnableChipSelect = callback;};
//void      SetPinCSFunct2  (void (*callback)(bool))              {cbEnableChipSelect2 = callback;};
private:
  uint8_t SPI_Status;
  InterfaceMode mode = MODE_SPI;

  // SPI
  SPIClass  *spi = nullptr;
  uint8_t   pinCS = SS;
  uint32_t  spiFreq = 4000000;
  std::function<void(bool)> cbEnableChipSelect = nullptr/*EnableChip*/;
//  void (*cbEnableChipSelect2)(bool) = EnableChip;

  // UART
  HardwareSerial *uart = nullptr;
  uint8_t uartAddr = 0;
  long uartBaud = 115200;

  // STEP/DIR
  uint8_t pinSTEP = 0xFF, pinDIR = 0xFF, pinEN = 0xFF;

  // Metodi privati
  void      spiWrite(Reg reg, uint32_t value);
  uint32_t  spiRead(Reg reg);
  void      sendUART(uint8_t addr, bool write, Reg reg, uint32_t data);
  bool      recvUART(uint8_t addr, Reg reg, uint32_t &data);
  void      enableRegBit(bool en, Reg reg, uint32_t Mask);

};

