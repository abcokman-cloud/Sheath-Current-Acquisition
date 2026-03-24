#include <stdint.h>

#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

/* --- Hardware Clock Configuration --- */
// Adjust the following values if crystal frequency changes (e.g., to 25MHz):
#define MCU_HSE_FREQ_HZ      8000000    // External crystal frequency
#define MCU_PLL_M            8          // PLL M divider

/* --- Sampling Configuration --- */
#define CH_NUM               8      // Number of channels
#define AC_FREQ_HZ           50     // AC Frequency
#define POINTS_PER_PERIOD    256    // Points per period
#define PHASE_POINTS_COUNT   256    // Alignment points

// --- Derived Constants ---
#define SAMPLE_FREQ_HZ       (AC_FREQ_HZ * POINTS_PER_PERIOD)    // Sampling frequency
#define TOTAL_DMA_SIZE       (CH_NUM * POINTS_PER_PERIOD)        // DMA transfer size

#define PERIODS_PER_SEC      AC_FREQ_HZ                          // Periods per second
#define SETTLE_PERIODS       PERIODS_PER_SEC                     // Settle periods
#define SAMPLE_CYCLES_PER_SEC AC_FREQ_HZ                         // Sample cycles per second

/* --- Global Configuration --- */

/* --- 1. Comm Defaults --- */
#define DEFAULT_MB_ID       1               // Modbus Slave ID
#define DEFAULT_BAUDRATE    115200          // Baudrate
#define TCP_PORT            502             // TCP Port

#define DEFAULT_MAC          {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33} 
#define DEFAULT_IP           {192, 168, 1, 100} 
#define DEFAULT_MASK         {255, 255, 255, 0} 
#define DEFAULT_GW           {192, 168, 1, 1}   

/* --- 2. Hardware params --- */
#define SAMPLES_PER_CH      10              // Samples per channel
#define HW_ADC_COEFF        0.0125f         

/* --- 3. Logic Defaults --- */
#define DEFAULT_CH_EN_1_4   1 							
#define DEFAULT_CH_EN_5_8   0 							
#define DEFAULT_CT_RATIO    1               
#define DEFAULT_CT_OFFSET   0               

/* --- Modbus Register Map --- */

/* Basic Info (R/W) */
#define MB_REG_DEV_ID           1           // Device ID
#define MB_REG_BAUDRATE_L       2           // Baudrate Low
#define MB_REG_BAUDRATE_H       3           // Baudrate High

/* Network Config (R/W) */
#define MB_REG_IP_START         11          // IP Start
#define MB_REG_MASK_START       15          // Mask Start
#define MB_REG_GW_START         19          // Gateway Start

/* Business Params (R/W, 32-bit Float) */
#define MB_REG_RATIO_START      31          // CT Ratio Start
#define MB_REG_OFFSET_START     51          // CT Offset Start

// Drift Parameters
#define MB_REG_SYS_XTAL_EN      67    // Drift enable
#define MB_REG_SYS_TEMP_VALUE   68    // Temperature
#define MB_REG_SYS_XTAL_DRIFT   69    // Drift value

/* Channel Control (R/W) */
#define MB_REG_CH_ENABLE_START  71          // Channel Enable Start

/* Command Bits */
#define MB_REG_SAVE_CMD         99          // Save Command
#define MB_REG_RESTORE_CMD      100         // Restore Command

// Default commands
#define DEFAULT_SAVE_CMD        0
#define DEFAULT_RESTORE_CMD     0

/* Real-time Data (Read-Only) */
#define MB_REG_RAW_RMS_START    81          // Raw RMS
#define MB_REG_REAL_CUR_START   101         // Real RMS

/* Logic Area */

// Unify logic
#define TOTAL_SAMPLES       (CH_NUM * SAMPLES_PER_CH) 

// Drift defaults
#define DEFAULT_XTAL_DRIFT    0.0f           
#define DEFAULT_XTAL_EN_STATE 0
#define DEFAULT_SYS_TEMP      25.0f   

// Sensor thresholds
#define SENSOR_ADC_NULL_MIN 300             
#define SENSOR_ADC_NULL_MAX 4000    

// Modbus registers array
extern uint16_t g_modbus_regs[512];

/* Function Interfaces */
uint8_t  SYS_Get_Ch_Enable(uint8_t ch);   // Get Channel Enable
float    SYS_Get_CT_Ratio(uint8_t ch);    // Get Ratio
void     SYS_Init(void);                  // Init system
void     SYS_Save_Config(void);           // Save to Flash
void     SYS_RestoreToFactory(void);      // Restore factory
void     SYS_Monitor_Cmd(void);           // Monitor commands

void     SYS_Push_Float_To_Modbus(uint16_t start_addr, uint8_t ch, float val_f);
void     SYS_Push_IEEE_Float_To_Modbus(uint16_t start_addr, uint8_t ch, float val_f);
float    SYS_Get_CT_Offset(uint8_t ch);   // Get Offset

/* Feature Values (Read-Only) */
#define MB_REG_PHASE_SHIFT_START   121      // Phase shift start
#define MB_REG_PEAK_VALLEY_DIST    131      // Peak-valley dist start

/* Vector Sum Area (Read-Only) */
#define MB_REG_VECTOR_SUM_START    141      // Vector sum
// Group 1 & 2

/* Power Quality Area (Read-Only) */
#define MB_REG_CREST_FACTOR_START  151      // Crest Factor
#define MB_REG_TRUE_DC_START       161      // True DC
#define MB_REG_GRID_FREQ_START     181      // Grid Frequency

/* Harmonic Diagnostic (Read-Only) */
#define MB_REG_THD_START           191      // THD
#define MB_REG_IHD2_START          201      // 2nd Harmonic
#define MB_REG_IHD3_START          211      // 3rd Harmonic
#define MB_REG_IHD5_START          221      // 5th Harmonic
#define MB_REG_IHD7_START          231      // 7th Harmonic

/* Time-domain Variance (Read-Only) */
#define MB_REG_PHASE_VAR_START     241      // Phase Variance
#define MB_REG_SYM_VAR_START       251      // Distortion Variance

/* Absolute Time-domain & Fundamental (Read-Only) */
#define MB_REG_PEAK_VAL_START      261      // Peak Value
#define MB_REG_PEAK_PHASE_START    271      // Peak Phase
#define MB_REG_VALLEY_VAL_START    281      // Valley Value
#define MB_REG_VALLEY_PHASE_START  291      // Valley Phase
#define MB_REG_ZERO_CROSS_START    301      // Zero Crossing Phase
#define MB_REG_FUNDAMENTAL_START   311      // Fundamental RMS

/* Global Communication Buffers */

extern uint8_t response_buf[5000];

// 8-channel waveform buffer
extern uint16_t g_waveform_data[CH_NUM][POINTS_PER_PERIOD];

#endif /* __SYS_CONFIG_H */
