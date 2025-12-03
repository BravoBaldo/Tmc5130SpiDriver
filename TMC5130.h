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

#define wxSIZEOF(a) (sizeof(a)/sizeof(a[0]))


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

  // ====== Registers ======
  typedef enum : uint8_t {
    GCONF       = 0x00,   // Gconf and GconfBits  RW
    GSTAT       = 0x01,   // Gstat                RC      Global status (write 1 to clear flags)
    IFCNT       = 0x02,   // 0xFF                 R      Interface counter
    NODECONF    = 0x03,   // Nodeconf             W      Slave config (UART)
    IOIN        = 0x04,   // Ioin                 RD      Input pins
    X_COMPARE   = 0x05,   //0xFFFF FFFF           W

    IHOLD_IRUN  = 0x10,   // IholdIrun            W      Corrente run/hold
    TPOWERDOWN  = 0x11,   //0x00FF                W
    TSTEP       = 0x12,   //0xFFFFF               R
    TPWMTHRS    = 0x13,   //0xFFFFF               W
    TCOOLTHRS   = 0x14,   //0xFFFFF               W
    THIGH       = 0x15,   //0xFFFFF               W
   
    RAMPMODE    = 0x20,   //0x03                  RW
    XACTUAL     = 0x21,   //0xFFFF FFFF           RW
    VACTUAL     = 0x22,   //0x00FF FFFF           R
    VSTART      = 0x23,   //0x0003 FFFF           W
    A1          = 0x24,   //0xFFFF                W
    V1          = 0x25,   //0x000F FFFF           W
    AMAX        = 0x26,   //0xFFFF                W
    VMAX        = 0x27,   //0x7F FFFF             W
    DMAX        = 0x28,   //0xFFFF                W

    D1          = 0x2A,   //0xFFFF                W
    VSTOP       = 0x2B,   //0x0003 FFFF           W
    TZEROWAIT   = 0x2C,   //0xFFFF                W
    XTARGET     = 0x2D,   //0xFFFF FFFF           RW

    VDCMIN      = 0x33,   //0x007F FFFF           W    Ramp Generator Driver Feature Control Register Set
    SW_MODE     = 0x34,   // SwMode               RW
    RAMP_STAT   = 0x35,   // RampStat             RC
    XLATCH      = 0x36,   //0xFFFF FFFF           R
    ENCMODE     = 0x38,   // Encmode              RW    Encoder Registers 
    X_ENC       = 0x39,   //0xFFFF FFFF           RW
    ENC_CONST   = 0x3A,   // EncConst             W
    ENC_STATUS  = 0x3B,   //0x0001                RC
    ENC_LATCH   = 0x3C,   //0xFFFF FFFF           R

    MSLUT_0     = 0x60,   //0sf[0...31]           W    Motor Driver Registers  - MICROSTEPPING CONTROL REGISTER SET
    MSLUT_1     = 0x61,   //0sf[32...63]          W
    MSLUT_2     = 0x62,   //0sf[64...95]          W
    MSLUT_3     = 0x63,   //0sf[96...127]         W
    MSLUT_4     = 0x64,   //0sf[128...159]        W
    MSLUT_5     = 0x65,   //0sf[160...191]        W
    MSLUT_6     = 0x66,   //0sf[192...223]        W
    MSLUT_7     = 0x67,   //0sf[224...255]        W
    MSLUTSEL    = 0x68,   // Mslutsel             W
    MSLUTSTART  = 0x69,   // Mslutstart           W
    MSCNT       = 0x6A,   //0x3FF                 R
    MSCURACT    = 0x6B,   // Mscuract             R
    CHOPCONF    = 0x6C,   // Chopconf             RW    DRIVER REGISTER SET    
    COOLCONF    = 0x6D,   // Coolconf             W
    DCCTRL      = 0x6E,   // Dcctrl               W
    DRV_STATUS  = 0x6F,   // DrvStatus            R

    PWMCONF     = 0x70,   // Pwmconf              W
    PWM_SCALE   = 0x71,   // PwmScale             R
    ENCM_CTRL   = 0x72,   // EncmCrtl             W
    LOST_STEPS  = 0x73,   //                      R
  }Reg;

 static constexpr Reg RegsReadable[] = {GCONF,GSTAT,IFCNT,IOIN,TSTEP,RAMPMODE,XACTUAL,VACTUAL,XTARGET,SW_MODE,
                                            RAMP_STAT,XLATCH,ENCMODE,X_ENC,ENC_STATUS,ENC_LATCH,MSCNT,MSCURACT,
                                            CHOPCONF,DRV_STATUS,PWM_SCALE,LOST_STEPS,
                                        };

  typedef enum : uint32_t { //GCONF
    GCONF_I_scale_analog		    = 0x00000001, //0000 0000 0000 0000 0000 0001
    GCONF_internal_Rsense		    = 0x00000002, //0000 0000 0000 0000 0000 0010
    GCONF_en_pwm_mode			      = 0x00000004, //0000 0000 0000 0000 0000 0100
    GCONF_enc_commutation		    = 0x00000008, //0000 0000 0000 0000 0000 1000
    GCONF_shaft					        = 0x00000010, //0000 0000 0000 0000 0001 0000
    //                                                               XX                                          
    GCONF_diag0_step			      = 0x00000080, //0000 0000 0000 0000 1000 0000
    GCONF_diag1_dir	            = 0x00000100, //0000 0000 0000 0001 0000 0000
    //                                                         XXX
    GCONF_diag0_int_pushpull	  = 0x00001000, //0000 0000 0001 0000 0000 0000
    GCONF_diag1_poscomp_pushpull= 0x00002000, //0000 0000 0010 0000 0000 0000
    GCONF_small_hysteresis		  = 0x00004000, //0000 0000 0100 0000 0000 0000
    GCONF_stop_enable			      = 0x00008000, //0000 0000 1000 0000 0000 0000
    GCONF_direct_mode			      = 0x00010000, //0000 0001 0000 0000 0000 0000
    GCONF_test_mode				      = 0x00020000, //0000 0010 0000 0000 0000 0000
    //                                XXXX XXXX XXXX XX
  }GconfBits; //  SetGconfBit, StopEnable
 

  typedef union  {    //GCONF
    struct {
      uint32_t recalibrate_i_scale_analog     : 1;  //0x01  0: Normal operation, use internal reference voltage 1: Use voltage supplied to AIN as current reference
      uint32_t faststandstill_internal_rsense : 1;  //0x02  0: Normal operation 1: Internal sense resistors. Use current supplied into AIN as reference for internal sense resistor
      uint32_t en_pwm_mode                    : 1;  //0x04  1: StealthChop voltage PWM mode enabled (depending on velocity thresholds). Switch from off to on state while in stand still, only
      uint32_t multistep_filt_enc_commutation : 1;  //0x08  (Special mode - do not use, leave 0)  1: Enable commutation by full step encoder (DCIN_CFG5 = ENC_A, DCEN_CFG4 = ENC_B)
      uint32_t shaft                          : 1;  //0x10  1: Inverse motor direction
      uint32_t diag0_error                    : 1;  //(only with SD_MODE=1) 1: Enable DIAG0 active on driver errors: Over temperature (ot), short to GND (s2g)  DIAG0 always shows the reset-status, i.e. is active low during reset condition.
      uint32_t diag0_otpw                     : 1;  //
      uint32_t diag0_stall_int_step           : 1;  //0x80
      uint32_t diag1_stall_poscomp_dir        : 1;  //0x100
      uint32_t diag1_index                    : 1;  //
      uint32_t diag1_onstate                  : 1;  //
      uint32_t diag1_steps_skipped            : 1;  //
      uint32_t diag0_int_pushpull             : 1;  //0x1000
      uint32_t diag1_poscomp_pushpull         : 1;  //0x2000
      uint32_t small_hysteresis               : 1;  //0x4000
      uint32_t stop_enable                    : 1;  //0x8000
      uint32_t direct_mode                    : 1;  //0x10000
      uint32_t test_mode                      : 1;  //0x20000
      uint32_t reserved                       : 14; //0x
    };
    uint32_t bytes;
  }Gconf;

  typedef union { //GSTAT
    struct {
      uint32_t reset    : 1;  //0x1
      uint32_t drv_err  : 1;  //0x2
      uint32_t uv_cp    : 1;  //0x4
      uint32_t reserved : 29;
    };
    uint32_t bytes;
  }Gstat;

  typedef union {  //MSLUTSEL
    struct {
      uint32_t W0    : 2;  //0x0000 0003  0000 0000 0000 0000 0000 0000 0000 0011
      uint32_t W1    : 2;  //0x0000 000C  0000 0000 0000 0000 0000 0000 0000 1100
      uint32_t W2    : 2;  //0x0000 0030  0000 0000 0000 0000 0000 0000 0011 0000
      uint32_t W3    : 2;  //0x0000 00C0  0000 0000 0000 0000 0000 0000 1100 0000
      uint32_t X1    : 8;  //0x0000 FF00  0000 0000 0000 0000 1111 1111 0000 0000
      uint32_t X2    : 8;  //0x00FF 0000  0000 0000 1111 1111 0000 0000 0000 0000
      uint32_t X3    : 8;  //0xFF00 0000  1111 1111 0000 0000 0000 0000 0000 0000
    };
    uint32_t bytes;
  }Mslutsel;

  typedef union {  //MSLUTSTART
    struct {
      uint32_t start_sin    : 8;  //0x00FF
      uint32_t reserved0    : 8;
      uint32_t start_sin90  : 8;  //0xFF 0000
      uint32_t reserved1    : 8;
    };
    uint32_t bytes;
  }Mslutstart;

  typedef union {
    struct {
      uint32_t cur_a      : 9;   //0x01FF
      uint32_t reserved0  : 7;
      uint32_t cur_b      : 9;  //0x01FF 0000
      uint32_t reserved1  : 7;
    };
    uint32_t bytes;
  }Mscuract;


  typedef union { //SLAVECONF
    struct {
      uint32_t nodeaddr   : 8;  //0x0FF
      uint32_t senddelay  : 4;  //0xF00
      uint32_t reserved   : 20; //
    };
    uint32_t bytes;
  }Nodeconf;


  typedef union { //ENCMODE
    struct {
      uint32_t pol_a            : 1;  //0x1
      uint32_t pol_b            : 1;  //0x2
      uint32_t pol_n            : 1;  //0x4
      uint32_t ignore_ab        : 1;  //0x8
      uint32_t clr_cont         : 1;  //0x10
      uint32_t clr_once         : 1;  //0x20
      uint32_t pos_edge         : 1;  //0x40
      uint32_t neg_edge         : 1;  //0x80
      uint32_t clr_enc_x        : 1;  //0x100
      uint32_t latch_x_act      : 1;  //0x200
      uint32_t enc_sel_decimal  : 1;  //0x400
      uint32_t reserved         : 21;
    };
    uint32_t bytes;
  }Encmode;


  typedef union { //ENC_CONST
    struct {
      uint32_t fractional : 16; //0x0000 FFFF
      uint32_t integer    : 16; //0xFFFF 0000
    };
    uint32_t bytes;
  }EncConst;


  typedef union { //PWM_SCALE
    struct {
      uint32_t pwm_scale_sum  : 8;  //0x00FF
      uint32_t reserved0      : 24;
    };
    uint32_t bytes;
  }PwmScale;

  typedef union { //ENCM_CTRL
    struct {
      uint32_t inv            : 1;  //0x0000 0001
      uint32_t maxspeed       : 1;  //0x0000 0002
      uint32_t reserved1      : 30;
    };
    uint32_t bytes;
  }EncmCrtl;


  typedef union { //RAMP_STAT
    struct {
      uint32_t status_stop_l      : 1;  //0x0001
      uint32_t status_stop_r      : 1;  //0x0002
      uint32_t status_latch_l     : 1;  //0x0004
      uint32_t status_latch_r     : 1;  //0x0008
      uint32_t event_stop_l       : 1;  //0x0010
      uint32_t event_stop_r       : 1;  //0x0020
      uint32_t event_stop_sg      : 1;  //0x0040
      uint32_t event_pos_reached  : 1;  //0x0080
      uint32_t velocity_reached   : 1;  //0x0100
      uint32_t position_reached   : 1;  //0x0200
      uint32_t vzero              : 1;  //0x0400
      uint32_t t_zerowait_active  : 1;  //0x0800
      uint32_t second_move        : 1;  //0x1000
      uint32_t status_sg          : 1;  //0x2000
      uint32_t reserved           : 18;
    };
    uint32_t bytes;
  }RampStat;

  typedef union { //DRV_STATUS
    struct {
      uint32_t sg_result  : 10; //0x 0000 03FF  00000000000000000000001111111111
      uint32_t reserved0  : 5;  //
      uint32_t fsactive   : 1;  //0x 0000 8000  00000000000000001000000000000000
      uint32_t cs_actual  : 5;  //0x 001F 0000  00000000000111110000000000000000
      uint32_t reserved1  : 3;  //
      uint32_t stallguard : 1;  //0x 0100 0000  00000001000000000000000000000000
      uint32_t ot         : 1;  //0x 0200 0000  00000010000000000000000000000000
      uint32_t otpw       : 1;  //0x 0400 0000  00000100000000000000000000000000
      uint32_t s2ga       : 1;  //0x 0800 0000  00001000000000000000000000000000
      uint32_t s2gb       : 1;  //0x 1000 0000  00010000000000000000000000000000
      uint32_t ola        : 1;  //0x 2000 0000  00100000000000000000000000000000
      uint32_t olb        : 1;  //0x 4000 0000  01000000000000000000000000000000
      uint32_t stst       : 1;  //0x 8000 0000  10000000000000000000000000000000
    };
    uint32_t bytes;
  }DrvStatus;

  typedef union  {  //SW_MODE
    struct {
      uint32_t stop_l_enable    : 1;	  //0x001    SW_MODE_STOP_L_ENABLE     
      uint32_t stop_r_enable    : 1;    //0x002    SW_MODE_STOP_R_ENABLE     
      uint32_t pol_stop_l       : 1;    //0x004    SW_MODE_POL_STOP_L        
      uint32_t pol_stop_r       : 1;    //0x008    SW_MODE_POL_STOP_R        
      uint32_t swap_lr          : 1;    //0x010    SW_MODE_SWAP_LR           
      uint32_t latch_l_active   : 1;    //0x020    SW_MODE_LATCH_L_ACTIVE    
      uint32_t latch_l_inactive : 1;    //0x040    SW_MODE_LATCH_L_INACTIVE  
      uint32_t latch_r_active   : 1;    //0x080    SW_MODE_LATCH_R_ACTIVE    
      uint32_t latch_r_inactive : 1;    //0x100    SW_MODE_LATCH_R_INACTIVE  
      uint32_t en_latch_encoder : 1;    //0x200    SW_MODE_EN_LATCH_ENCODER  
      uint32_t sg_stop          : 1;    //0x400    SW_MODE_SG_STOP           
      uint32_t en_softstop      : 1;    //0x800    SW_MODE_EN_SOFTSTOP       
      uint32_t reserved         : 20;
    };
    uint32_t bytes;
  }SwMode;



  typedef union {   //PWMCONF
    struct {
      uint32_t pwm_ampl       : 8;  //0x0000 00FF    alias pwm_ofs
      uint32_t pwm_grad       : 8;  //0x0000 FF00
      uint32_t pwm_freq       : 2;  //0x0003 0000
      uint32_t pwm_autoscale  : 1;  //0x0004 0000
      uint32_t pwm_symmetric  : 1;  //0x0008 0000     alias pwm_autograd
      uint32_t freewheel      : 2;  //0x0030 0000
      uint32_t reserved0      : 2;  //0x
      uint32_t reserved1      : 4;  //0x     pwm_reg does not exists in 5130
      uint32_t reserved2      : 4;  //0x     pwm_lim does not exists in 5130
    };
    uint32_t bytes;
  }Pwmconf;





  typedef union { //IHOLD_IRUN
    struct {
      uint32_t ihold      : 5;  //0x1F
      uint32_t reserved0  : 3;
      uint32_t irun       : 5;  //0x1F00
      uint32_t reserved1  : 3;
      uint32_t iholddelay : 4;  //0xF0000
      uint32_t reserved2  : 12;
    };
    uint32_t bytes;
  }IholdIrun;

  typedef union  {  //CHOPCONF dd
    struct {
      uint32_t toff           : 4;  //0x 0000000F 00000000000000000000000000001111
      uint32_t HSTRT          : 3;  //0x 00000070 00000000000000000000000001110000
      uint32_t HEND           : 4;  //0x 00000780 00000000000000000000011110000000
      uint32_t fd3            : 1;  //0x 00001800 00000000000000000000100000000000
      uint32_t disfdcc        : 1;  //0x 00001800 00000000000000000001000000000000
      uint32_t rndtf          : 1;  //0x 00002000 00000000000000000010000000000000
      uint32_t chm            : 1;  //0x 00004000 00000000000000000100000000000000
      uint32_t tbl            : 2;  //0x 00018000 00000000000000011000000000000000
      uint32_t vsense         : 1;  //0x 00020000 00000000000000100000000000000000
      uint32_t vhighfs        : 1;  //0x 00040000 00000000000001000000000000000000
      uint32_t vhighchm       : 1;  //0x 00080000 00000000000010000000000000000000
      uint32_t sync           : 4;  //0x 00F00000 00000000111100000000000000000000
      uint32_t mres           : 4;  //0x 0F000000 00001111000000000000000000000000
      uint32_t interpol       : 1;  //0x 10000000 00010000000000000000000000000000
      uint32_t dedge          : 1;  //0x 20000000 00100000000000000000000000000000
      uint32_t diss2g         : 1;  //0x 40000000 01000000000000000000000000000000
      uint32_t reserved2      : 1;  //0x 80000000 10000000000000000000000000000000
    };
    uint32_t bytes;
  }Chopconf;


  typedef union { //COOLCONF
    struct {
      uint32_t semin      : 4;  //0x0000 000F 00000000000000000000000000001111
      uint32_t reserved0  : 1;  //            000000000000000000000000000X0000
      uint32_t seup       : 2;  //0x0000 0060 00000000000000000000000001100000
      uint32_t reserved1  : 1;  //            000000000000000000000000X0000000
      uint32_t semax      : 4;  //0x0000 0F00 00000000000000000000111100000000
      uint32_t reserved2  : 1;  //            0000000000000000000X000000000000
      uint32_t sedn       : 2;  //0x0000 6000 00000000000000000110000000000000
      uint32_t seimin     : 1;  //0x0000 8000 00000000000000001000000000000000
      uint32_t sgt        : 7;  //0x007F 0000 00000000011111110000000000000000
      uint32_t reserved3  : 1;  //            00000000X00000000000000000000000
      uint32_t sfilt      : 1;  //0x0100 0000 00000001000000000000000000000000
      uint32_t reserved4  : 7;  //            XXXXXXX0000000000000000000000000
    };
    uint32_t bytes;
  }Coolconf;

  typedef union { //DCCTRL
    struct {
      uint32_t dc_time    : 10; //0x0000 03FF
      uint32_t reserved0  : 6;  //
      uint32_t dc_sg      : 8;  //0x00FF 0000
      uint32_t reserved1  : 8;  //
    };
    uint32_t bytes;
  } Dcctrl;

  typedef enum { HardMode = 0, SoftMode = 1,}   StopMode;

  typedef union { //IOIN
    struct {
      uint32_t refl_step      : 1;  //0x1
      uint32_t refr_dir       : 1;  //0x2
      uint32_t encb_dcen_cfg4 : 1;  //0x4
      uint32_t enca_dcin_cfg5 : 1;  //0x8
      uint32_t drv_enn        : 1;  //0x10
      uint32_t enc_n_dco_cfg6 : 1;  //0x20
      uint32_t sd_mode        : 1;  //0x40
      uint32_t swcomp_in      : 1;  //0x80
      uint32_t reserved       : 16;
      uint32_t version        : 8;  //0xFF00 0000
    };
    uint32_t bytes;
  }Ioin;

  typedef enum : uint8_t {
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

struct ConverterParameters {
  uint8_t   clock_frequency_mhz               = 12;
  uint32_t  microsteps_per_real_position_unit = 1;
  uint32_t  seconds_per_real_velocity_unit    = 1;
};

struct HomeParameters{
  uint8_t run_current         = 0;
  uint8_t hold_current        = 0;
  int32_t target_position     = 0;
  uint32_t velocity           = 0;
  uint32_t acceleration       = 0;
  uint32_t zero_wait_duration = 100;
};

struct StallParameters {
  int8_t stall_guard_threshold = 0;
  uint32_t cool_step_threshold = 0;
};


void cacheControllerSettings(ControllerParameters &Ret);
void writeControllerParameters(ControllerParameters &parameters);

  typedef enum { ForwardDirection = 0, ReverseDirection = 1}                                          MotorDirection;
  typedef enum { NormalMode=0, FreewheelingMode=1, PassiveBrakingLsMode=2, PassiveBrakingHsMode=3 }   StandstillMode;
  typedef enum { SpreadCycleMode = 0, ClassicMode = 1 }                                               ChopperMode;
  typedef enum { ClockCycles16 = 0, ClockCycles24 = 1, ClockCycles36 = 2, ClockCycles54 = 3 }         ComparatorBlankTime;


//To read Writable only registers
struct DriverParameters { // global_current_scaler only available on TMC5160
  uint8_t             run_current                           = 50;               //IHOLD_IRUN
  uint8_t             hold_current                          = 20;               //IHOLD_IRUN
  uint8_t             hold_delay                            = 5;                //IHOLD_IRUN
  uint8_t             pwm_offset                            = 25;               //PWMCONF
  uint8_t             pwm_gradient                          = 5;                //PWMCONF
  bool                automatic_current_control_enabled     = false;            //PWMCONF
  MotorDirection      motor_direction                       = ForwardDirection; //GCONF RW
  StandstillMode      standstill_mode                       = NormalMode;       //GCONF RW
  ChopperMode         chopper_mode                          = SpreadCycleMode;  //CHOPCONF RW
  uint32_t            stealth_chop_threshold                = 100;              //CHOPCONF RW
  bool                stealth_chop_enabled                  = true;             //CHOPCONF RW
  uint32_t            cool_step_threshold                   = 150;              //CHOPCONF RW
  uint8_t             cool_step_min                         = 1;                //COOLCONF
  uint8_t             cool_step_max                         = 0;                //COOLCONF
  bool                cool_step_enabled                     = false;            //COOLCONF
  uint32_t            high_velocity_threshold               = 200;              //COOLCONF
  bool                high_velocity_fullstep_enabled        = false;            //COOLCONF
  bool                high_velocity_chopper_switch_enabled  = false;            //COOLCONF
  int8_t              stall_guard_threshold                 = 0;                //COOLCONF
  bool                stall_guard_filter_enabled            = false;            //COOLCONF
  bool                short_to_ground_protection_enabled    = true;             //COOLCONF
  uint8_t             enabled_toff                          = 3;                //COOLCONF
  ComparatorBlankTime comparator_blank_time                 = ClockCycles36;    //COOLCONF
  uint16_t            dc_time                               = 0;                //DCCTRL
  uint8_t             dc_stall_guard_threshold              = 0;                //DCCTRL
};


struct {
//    SLAVECONF   = 0x03,   // Nodeconf             W      Slave config (UART)
//    X_COMPARE   = 0x05,   //0xFFFF FFFF           W

  IholdIrun Ihold_Irun;     //IHOLD_IRUN  = 0x10,   // IholdIrun            W      Corrente run/hold
  uint32_t  TPowerDown;     //TPOWERDOWN  = 0x11,   //0x00FF                W
/*
    TPWMTHRS    = 0x13,   //0xFFFFF               W
    TCOOLTHRS   = 0x14,   //0xFFFFF               W
    THIGH       = 0x15,   //0xFFFFF               W   
    VSTART      = 0x23,   //0x0003 FFFF           W
    A1          = 0x24,   //0xFFFF                W
    V1          = 0x25,   //0x000F FFFF           W
    AMAX        = 0x26,   //0xFFFF                W
    VMAX        = 0x27,   //0x7F FFFF             W
    DMAX        = 0x28,   //0xFFFF                W
    D1          = 0x2A,   //0xFFFF                W
    VSTOP       = 0x2B,   //0x0003 FFFF           W
    TZEROWAIT   = 0x2C,   //0xFFFF                W
    VDCMIN      = 0x33,   //0x007F FFFF           W
    ENC_CONST   = 0x3A,   // EncConst             W
    MSLUT_0     = 0x60,   //0sf[0...31]           W
    MSLUT_1     = 0x61,   //0sf[32...63]          W
    MSLUT_2     = 0x62,   //0sf[64...95]          W
    MSLUT_3     = 0x63,   //0sf[96...127]         W
    MSLUT_4     = 0x64,   //0sf[128...159]        W
    MSLUT_5     = 0x65,   //0sf[160...191]        W
    MSLUT_6     = 0x66,   //0sf[192...223]        W
    MSLUT_7     = 0x67,   //0sf[224...255]        W
    MSLUTSEL    = 0x68,   // Mslutsel             W
    MSLUTSTART  = 0x69,   // Mslutstart           W
*/
    Coolconf    CoolConf;   //COOLCONF    = 0x6D,   // Coolconf             W
    Dcctrl      DcCtrl;     //DCCTRL      = 0x6E,   // Dcctrl               W
    Pwmconf     PwmConf;    //PWMCONF     = 0x70,   // Pwmconf              W
//    ENCM_CTRL   = 0x72,   // EncmCrtl             W

  uint32_t  MaxVelocity;
}ShadowRegs;



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
  void setCurrent       (uint8_t irun, uint8_t ihold, uint8_t holdDelay); //IHOLD_IRUN
  inline void setNeutral       (void)  {
      setCurrent(0, 0, 0);                      //IHOLD_IRUN
      SetGconfBit(GCONF_en_pwm_mode, false);     //GCONF     en_pwm_mode = 1; //Enable StealthChop
      writeStandstillMode( FreewheelingMode );  //PWMCONF
    };
  inline void setParking       (void)           { setCurrent(31, 31, 15); };   //IHOLD_IRUN
  void        setMicrosteps    (uint8_t mres);

  inline uint8_t getMicrosteps  (void)  {return (readReg(CHOPCONF)>>24) & 0x0F;}
  void softwareEnable           (void);  //CHOPCONF

  void            SetGconfBit               (GconfBits b, bool en); //GCONF
  inline void     StopEnable                (bool en) { SetGconfBit(GCONF_stop_enable, en); }
  inline void     enableInverseDirection    (bool en) { enableRegBit(en, GCONF, 0x00010); }
  inline void     enableStopEnable          (bool en) { enableRegBit(en, GCONF, 0x08000); }
  inline void     enableDirectMode          (bool en) { enableRegBit(en, GCONF, 0x10000); }
  inline uint8_t  GetSpiStatus              (Reg reg=GCONF, uint32_t value=0, bool Read=true) { 
      //genSpiFunct(reg, value, Read);  //Interferisce col motore!!!!!
      readReg(reg);
      return SPI_Status;
    };
  inline void     setGconf                  (uint32_t gconf) { writeReg(GCONF, gconf); }

  inline void     setChopconf       (uint32_t chopconf) { writeReg(CHOPCONF, chopconf); }

  inline void     setPwmconf        (uint32_t pwmconf)  { writeReg(PWMCONF, pwmconf); ShadowRegs.PwmConf.bytes = pwmconf; }
  inline uint32_t getPwmconf        (void)              { return ShadowRegs.PwmConf.bytes; }

  inline void     setCoolconf       (uint32_t coolconf) { writeReg(COOLCONF, coolconf); ShadowRegs.CoolConf.bytes = coolconf; }
  inline uint32_t getCoolconf       (void)              { return ShadowRegs.CoolConf.bytes; }

  inline void     setTPowerDown     (uint32_t tpwdwn)   { writeReg(TPOWERDOWN, tpwdwn); ShadowRegs.TPowerDown = tpwdwn; } //TPOWERDOWN
  inline uint32_t getTPowerDown     (void)              { return ShadowRegs.TPowerDown; }                                 //TPOWERDOWN


  inline void     setDcctrl         (uint32_t dcctrl) { writeReg(DCCTRL, dcctrl); ShadowRegs.DcCtrl.bytes = dcctrl; }
  inline uint32_t getDcctrl         (void)              { return ShadowRegs.DcCtrl.bytes; }

  void setDcStep        (uint16_t dcTime, uint16_t dcSG);
  void setTCOOLTHRS     (uint32_t val);

  uint16_t  readStallGuardResult        (void);   //DRV_STATUS

  // Motion controller
  void setRampMode        (RampMode m);
  inline RampMode getRampMode (void)  { return (RampMode)(readReg(RAMPMODE) & 0x3); };



  void setMaxVelocity             (uint32_t v);  //0...8388096=7FFE00
  inline uint32_t getMaxVelocity  (void)  {  return ShadowRegs.MaxVelocity; };

  void setStartVelocity  (uint32_t v);  //0...262143=3FFFF  //Motor start velocity
  void setFirstVelocity  (uint32_t v);  //0...1048575=FFFFF
  void setStopVelocity   (uint32_t v);  //1...262143=3FFFF  //Motor stop velocity (unsigned)

  inline int32_t getVelocity    (void)            {return ((int32_t)readReg(VACTUAL  )<<8)>>8;}
  bool zeroVelocity             (void);
  bool positionReached          (void);
  void beginRampToZeroVelocity  (void);
  bool homed                    (void);
  void setFirstAcceleration   (uint16_t a);  //A1    [μsteps / ta²]  0...1048575=0xFFFFF First acceleration between VSTART and V1 (unsigned)
  void setSecondAcceleration  (uint16_t a);  //AMAX  [μsteps / ta²]  0...1048575=0xFFFFF Second acceleration between V1 and VMAX (unsigned)
  void setFirstDeceleration   (uint16_t d);  //DMAX  [μsteps / ta²]  0...1048575=0xFFFFF Deceleration between VMAX and V1 (unsigned)
  void setSecondDeceleration  (uint16_t d);  //D1    [μsteps / ta²]  0...1048575=0xFFFFF Deceleration between V1 and VSTOP (unsigned)

  inline void     setPosition (int32_t x)       { writeReg(XACTUAL, x); }
  inline int32_t  getPosition (void)            { return (int32_t)readReg(XACTUAL); }

  inline void     moveTo      (int32_t xTarget) { writeReg(XTARGET, (uint32_t)xTarget); setRampMode(PositionMode); }
  inline void     moveBy      (int32_t dx)      { moveTo(getPosition() + dx); }

  inline void     StopMotor   (uint16_t a=10) { //Method A
      setRampMode(VelocityPositiveMode);  //RAMPMODE
      setSecondAcceleration(a);           //AMAX
      setMaxVelocity(0);                  //VMAX
  }

  void  SetTrapezoidal(uint16_t a, uint32_t v){
    setSecondAcceleration  (a);  //AMAX  [μsteps / ta²]  0...1048575=0xFFFFF Second acceleration between V1 and VMAX (unsigned)
    setFirstDeceleration   (a);  //DMAX  [μsteps / ta²]  0...1048575=0xFFFFF Deceleration between VMAX and V1 (unsigned)
    setMaxVelocity         (v);  //VMAX  0...8388096=7FFE00
    writeReg(TZEROWAIT, 0);
  }

  inline void     StopMotorB   (void) { writeReg(VSTART, 0); writeReg(VMAX, 0); } //Method B

  inline uint8_t  getIcVersion  (void)          { return (readReg(IOIN)>>24) & 0xFF; }
  uint8_t         stepAndDirectionMode  (void);    //IOIN
  inline bool     IsConnected   (void)          { return getIcVersion()==0x11;}

  void writeStopMode            (StopMode stop_mode); //SW_MODE
  void enableStallStop          (bool En);            //SW_MODE //True=enable, False=disable
  void endHome                  (void);

  // ====== Utilities Step/Dir ======
  void stepOnce           (void);
  void setDirection       (bool dirCW);
  void enableDriver       (bool en);

  int32_t   Init_MicroSteps (uint8_t ms);
  inline uint8_t   GetLastSpiStatus   (void)                                            {return SPI_Status;}

  uint32_t  genSpiFunct     (Reg reg, uint32_t value, bool Read);

  void      SetPinCSFunct   (SPI_ENABLER_CB callback)                      {cbEnableChipSelect = callback;};
//void      SetPinCSFunct2  (void (*callback)(uint8_t, bool))              {cbEnableChipSelect2 = callback;};

  char*     GetName         (void)  {return StepName;}
  void      SetName         (char* n)  {StepName=n;}

  void writeStandstillMode           (StandstillMode mode);    //PWMCONF
  void writePwmOffset                (uint8_t pwm_amplitude);  //PWMCONF
  void writePwmGradient              (uint8_t pwm_amplitude);  //PWMCONF
  void enableAutomaticCurrentControl (bool en);                //PWMCONF

  void setMotorDirection(MotorDirection motor_direction) {  //GCONF  True=1=ReverseDirection, false=0=ForwardDirection
    Gconf gconf;
    gconf.bytes = readReg(GCONF);
    gconf.shaft = motor_direction;
    setGconf(gconf.bytes);
  }

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

