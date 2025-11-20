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

typedef std::function<void(uint8_t, bool)> SPI_ENABLER_CB;

extern void EnableSpiOnChip(uint8_t csPin, bool en);
//extern SPI_ENABLER_CB EnableSpiOnChip;

class TMC5130 {
public:
  // ====== Modalità operative ======
  enum InterfaceMode {
    MODE_SPI,
    MODE_UART,    //NOT TESTED
    MODE_STEPDIR  //NOT TESTED
  };

  //MicroSteps Mask
  typedef enum : uint32_t {
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
/*
    OtpProgAddress = 0x06,
    OtpReadAddress = 0x07,
    FactoryConfAddress = 0x08,
    ShortConfAddress = 0x09,
    DrvConfAddress = 0x0A,
    GlobalScalerAddress = 0x0B,
    OffsetReadAddress = 0x0C,

*/
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
    PWM_SCALE   = 0x71,
    ENCM_CTRL   = 0x72,
    LOST_STEPS  = 0x73,
  };

  typedef union { //PWM_SCALE
    struct {
      uint32_t pwm_scale_sum : 8;
      uint32_t reserved0 : 24;
    };
    uint32_t bytes;
  }PwmScale;

  typedef union { //RAMP_STAT
    struct {
      uint32_t status_stop_l      : 1;
      uint32_t status_stop_r      : 1;
      uint32_t status_latch_l     : 1;
      uint32_t status_latch_r     : 1;
      uint32_t event_stop_l       : 1;
      uint32_t event_stop_r       : 1;
      uint32_t event_stop_sg      : 1;
      uint32_t event_pos_reached  : 1;
      uint32_t velocity_reached   : 1;
      uint32_t position_reached   : 1;
      uint32_t vzero              : 1;
      uint32_t t_zerowait_active  : 1;
      uint32_t second_move        : 1;
      uint32_t status_sg          : 1;
      uint32_t reserved           : 18;
    };
    uint32_t bytes;
  }RampStat;


  typedef enum : uint32_t {
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

  typedef enum : uint16_t { //SW_MODE
    SW_MODE_STOP_L_ENABLE     = 0x001,	//stop_l_enable    
    SW_MODE_STOP_R_ENABLE     = 0x002,  //stop_r_enable    
    SW_MODE_POL_STOP_L        = 0x004,  //pol_stop_l       
    SW_MODE_POL_STOP_R        = 0x008,  //pol_stop_r       
    SW_MODE_SWAP_LR           = 0x010,  //swap_lr          
    SW_MODE_LATCH_L_ACTIVE    = 0x020,  //latch_l_active   
    SW_MODE_LATCH_L_INACTIVE  = 0x040,  //latch_l_inactive 
    SW_MODE_LATCH_R_ACTIVE    = 0x080,  //latch_r_active   
    SW_MODE_LATCH_R_INACTIVE  = 0x100,  //latch_r_inactive 
    SW_MODE_EN_LATCH_ENCODER  = 0x200,  //en_latch_encoder 
    SW_MODE_SG_STOP           = 0x400,  //sg_stop          
    SW_MODE_EN_SOFTSTOP       = 0x800,  //en_softstop      
  }SwModes;

  typedef union  {  //SW_MODE
    struct {
      uint32_t stop_l_enable    : 1;	  //SW_MODE_STOP_L_ENABLE     
      uint32_t stop_r_enable    : 1;    //SW_MODE_STOP_R_ENABLE     
      uint32_t pol_stop_l       : 1;    //SW_MODE_POL_STOP_L        
      uint32_t pol_stop_r       : 1;    //SW_MODE_POL_STOP_R        
      uint32_t swap_lr          : 1;    //SW_MODE_SWAP_LR           
      uint32_t latch_l_active   : 1;    //SW_MODE_LATCH_L_ACTIVE    
      uint32_t latch_l_inactive : 1;    //SW_MODE_LATCH_L_INACTIVE  
      uint32_t latch_r_active   : 1;    //SW_MODE_LATCH_R_ACTIVE    
      uint32_t latch_r_inactive : 1;    //SW_MODE_LATCH_R_INACTIVE  
      uint32_t en_latch_encoder : 1;    //SW_MODE_EN_LATCH_ENCODER  
      uint32_t sg_stop          : 1;    //SW_MODE_SG_STOP           
      uint32_t en_softstop      : 1;    //SW_MODE_EN_SOFTSTOP       
      uint32_t reserved         : 20;
    };
    uint32_t bytes;
  }SwMode;

  typedef union  {    //GCONF
    struct {
      uint32_t recalibrate_i_scale_analog : 1;
      uint32_t faststandstill_internal_rsense : 1;
      uint32_t en_pwm_mode : 1;
      uint32_t multistep_filt_enc_commutation : 1;
      uint32_t shaft : 1;
      uint32_t diag0_error : 1;
      uint32_t diag0_otpw : 1;
      uint32_t diag0_stall_int_step : 1;
      uint32_t diag1_stall_poscomp_dir : 1;
      uint32_t diag1_index : 1;
      uint32_t diag1_onstate : 1;
      uint32_t diag1_steps_skipped : 1;
      uint32_t diag0_int_pushpull : 1;
      uint32_t diag1_poscomp_pushpull : 1;
      uint32_t small_hysteresis : 1;
      uint32_t stop_enable : 1;
      uint32_t direct_mode : 1;
      uint32_t reserved : 15;
    };
    uint32_t bytes;
  }Gconf;



  typedef union {   //PWMCONF
    struct {
      uint32_t pwm_ampl : 8;  //alias pwm_ofs
      uint32_t pwm_grad : 8;
      uint32_t pwm_freq : 2;
      uint32_t pwm_autoscale : 1;
      uint32_t pwm_symmetric : 1;
      uint32_t freewheel : 2;
      uint32_t reserved0 : 2;
      uint32_t reserved1 : 4;
      uint32_t reserved2 : 4;
    };
    uint32_t bytes;
  }Pwmconf;



  typedef enum { ForwardDirection = 0, ReverseDirection = 1}                                          MotorDirection;
  typedef enum { NormalMode=0, FreewheelingMode=1, PassiveBrakingLsMode=2, PassiveBrakingHsMode=3 }   StandstillMode;
  typedef enum { SpreadCycleMode = 0, ClassicMode = 1 }                                               ChopperMode;
  typedef enum { ClockCycles16 = 0, ClockCycles24 = 1, ClockCycles36 = 2, ClockCycles54 = 3 }         ComparatorBlankTime;



  typedef union { //IHOLD_IRUN
    struct {
      uint32_t ihold : 5;
      uint32_t reserved0 : 3;
      uint32_t irun : 5;
      uint32_t reserved1 : 3;
      uint32_t iholddelay : 4;
      uint32_t reserved2 : 12;
    };
    uint32_t bytes;
  }IholdIrun;

  typedef union  {  //CHOPCONF
    struct {
      uint32_t toff : 4;
      uint32_t hstart : 3;
      uint32_t hend : 4;
      uint32_t fd3 : 1;
      uint32_t disfdcc : 1;
      uint32_t reserved_0 : 1;
      uint32_t chm : 1;
      uint32_t tbl : 2;
      uint32_t reserved_1 : 1;
      uint32_t vhighfs : 1;
      uint32_t vhighchm : 1;
      uint32_t tpfd : 4;
      uint32_t mres : 4;
      uint32_t interpolation : 1;
      uint32_t double_edge : 1;
      uint32_t diss2g : 1;
      uint32_t diss2vs : 1;
    };
    uint32_t bytes;
  }Chopconf;

  typedef union { //COOLCONF
    struct {
      uint32_t semin : 4;
      uint32_t reserved0 : 1;
      uint32_t seup : 2;
      uint32_t reserved1 : 1;
      uint32_t semax : 4;
      uint32_t reserved2 : 1;
      uint32_t sedn : 2;
      uint32_t seimin : 1;
      uint32_t sgt : 7;
      uint32_t reserved3 : 1;
      uint32_t sfilt : 1;
      uint32_t reserved4 : 7;
    };
    uint32_t bytes;
  }Coolconf;

  typedef union { //DCCTRL
    struct {
      uint32_t dc_time : 10;
      uint32_t reserved0 : 6;
      uint32_t dc_sg : 8;
      uint32_t reserved1 : 8;
    };
    uint32_t bytes;
  } Dcctrl;

  typedef enum { HardMode = 0, SoftMode = 1,}   StopMode;

  typedef union { //IOIN
    struct {
      uint32_t refl_step : 1;
      uint32_t refr_dir : 1;
      uint32_t encb_dcen_cfg4 : 1;
      uint32_t enca_dcin_cfg5 : 1;
      uint32_t drv_enn : 1;
      uint32_t enc_n_dco_cfg6 : 1;
      uint32_t sd_mode : 1;
      uint32_t swcomp_in : 1;
      uint32_t reserved : 16;
      uint32_t version : 8;
    };
    uint32_t bytes;
  }Ioin;

  typedef enum : uint8_t {
    TMC5130_MODE_POSITION  = 0,  //Positioning mode (using all A, D and V parameters)
    TMC5130_MODE_VELPOS    = 1,  //Velocity mode to positive VMAX (using AMAX acceleration)
    TMC5130_MODE_VELNEG    = 2,  //Velocity mode to negative VMAX (using AMAX acceleration)
    TMC5130_MODE_HOLD      = 3,  //Hold mode (velocity remains unchanged, unless stop event occurs)
  }TMC5130_RampMode;


  typedef enum {
    PositionMode          = 0,  //Positioning mode (using all A, D and V parameters)
    VelocityPositiveMode  = 1,  //Velocity mode to positive VMAX (using AMAX acceleration)
    VelocityNegativeMode  = 2,  //Velocity mode to negative VMAX (using AMAX acceleration)
    HoldMode              = 3,  //Hold mode (velocity remains unchanged, unless stop event occurs)
  }RampMode;

  typedef struct {
    RampMode ramp_mode            = VelocityPositiveMode;
    StopMode stop_mode            = HardMode;
    uint32_t max_velocity         = 10;
    uint32_t max_acceleration     = 10;
    uint32_t start_velocity       = 1;
    uint32_t stop_velocity        = 10;
    uint32_t first_velocity       = 0;
    uint32_t first_acceleration   = 0;
    uint32_t max_deceleration     = 0;
    uint32_t first_deceleration   = 10;
    uint32_t zero_wait_duration   = 0;
    bool stall_stop_enabled       = false;
    uint32_t min_dc_step_velocity = 0;
  }ControllerParameters;

void cacheControllerSettings(ControllerParameters &Ret);

  // ====== Ctors ======
  TMC5130(SPIClass &spiRef, uint8_t csPin, SPI_ENABLER_CB cbCS=EnableSpiOnChip, uint32_t spiHz = 1000000, char* Name=nullptr);

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
  inline RampMode getRampMode (void)  { return (RampMode)(readReg(TMC5130::RAMPMODE) & 0x3); };
  void setMaxVelocity    (uint32_t v);  //0...8388096=7FFE00
  void setStartVelocity  (uint32_t v);  //0...262143=3FFFF  //Motor start velocity
  void setFirstVelocity  (uint32_t v);  //0...1048575=FFFFF
  void setStopVelocity   (uint32_t v);  //1...262143=3FFFF  //Motor stop velocity (unsigned)

  inline int32_t getVelocity    (void)            {return ((int32_t)readReg(VACTUAL  )<<8)>>8;}
  bool zeroVelocity             (void);
  void beginRampToZeroVelocity  (void);

  void setFirstAcceleration   (uint16_t amax);  //[μsteps / ta²]  0...1048575=0xFFFFF First acceleration between VSTART and V1 (unsigned)
  void setSecondAcceleration  (uint16_t amax);  //[μsteps / ta²]  0...1048575=0xFFFFF Second acceleration between V1 and VMAX (unsigned)
  void setFirstDeceleration   (uint16_t dmax);  //[μsteps / ta²]  0...1048575=0xFFFFF Deceleration between VMAX and V1 (unsigned)
  void setSecondDeceleration  (uint16_t dmax);  //[μsteps / ta²]  0...1048575=0xFFFFF Deceleration between V1 and VSTOP (unsigned)

  inline void     setPosition (int32_t x)       { writeReg(XACTUAL, x); }
  inline int32_t  getPosition (void)            { return (int32_t)readReg(XACTUAL); }

  inline void     moveTo      (int32_t xTarget) { writeReg(XTARGET, (uint32_t)xTarget); setRampMode(TMC5130_MODE_POSITION); }
  inline void     moveBy      (int32_t dx)      { moveTo(getPosition() + dx); }

  inline void     hardStop      (void)          { setRampMode(TMC5130_MODE_HOLD); writeReg(VMAX, 0); }
  inline uint8_t  getIcVersion  (void)          { return (readReg(IOIN)>>24) & 0xFF; }
  inline bool     communicating (void)          { return getIcVersion()==0x11;}

  // ====== Utilities Step/Dir ======
  void stepOnce           (void);
  void setDirection       (bool dirCW);
  void enableDriver       (bool en);
  void SetSwMode          (SwModes m);

  int32_t   Init_MicroSteps (uint8_t ms);
  inline uint8_t   GetLastSpiStatus   (void)                                            {return SPI_Status;}
  inline uint8_t   GetSpiStatus       (Reg reg=GCONF, uint32_t value=0, bool Read=true) { 
      //genSpiFunct(reg, value, Read);  //Interferisce col motore!!!!!
      readReg(reg);
      return SPI_Status;
    };

  uint32_t  genSpiFunct     (Reg reg, uint32_t value, bool Read);

  void      SetPinCSFunct   (SPI_ENABLER_CB callback)                      {cbEnableChipSelect = callback;};
//void      SetPinCSFunct2  (void (*callback)(uint8_t, bool))              {cbEnableChipSelect2 = callback;};

  char*     GetName         (void)  {return StepName;}
  void      SetName         (char* n)  {StepName=n;}
private:

  // SPI
  uint8_t       SPI_Status;       //Last SPI status register
  InterfaceMode mode = MODE_SPI;
  uint8_t       csPinAddress;
  SPIClass      *spi = nullptr;
  uint8_t       pinCS = SS;
  uint32_t      spiFreq = 4000000;
  SPI_ENABLER_CB cbEnableChipSelect = nullptr/*EnableChip*/;
//  void (*cbEnableChipSelect2)(uint8_t, bool) = EnableChip;
  char*         StepName=nullptr;
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

