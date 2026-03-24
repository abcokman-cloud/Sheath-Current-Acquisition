#ifndef __DSP_ALGO_H
#define __DSP_ALGO_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

// Status codes
#define STATUS_OK             0x00
#define STATUS_HARM_ALARM     0x01 
#define STATUS_FREQ_ALARM     0x02 

/* --- Instant Calculation Result --- */
typedef struct {
    float rms;                // Instant RMS
    float phase_align_shift;  // Phase alignment shift
    uint8_t status;           // Status flag
} MonitorResult;

/* --- Global Statistics Result --- */
typedef struct {
	float total_rms;                     // Total RMS over 1s
	float *point_rms;                    // Points RMS pointer
	uint8_t harmonic_status;             // Peak-valley mismatch distance
	float phase_variance;                // Phase variance
	float sym_distort_variance;          // Symmetric distortion variance
	float crest_factor;                  // Crest factor
	float true_dc_value;                 // True DC value
	float grid_frequency;                // Grid frequency
	
	float peak_val;                      // Peak absolute value
	float peak_phase;                    // Peak phase (0~360)
	float valley_val;                    // Valley absolute value
	float valley_phase;                  // Valley phase (0~360)
	float zero_crossing_phase;           // Zero-crossing phase

	float fundamental_rms;               // Fundamental RMS
	float thd;                           // THD
	float ihd_2;                         // 2nd harmonic
	float ihd_3;                         // 3rd harmonic
	float ihd_5;                         // 5th harmonic
	float ihd_7;                         // 7th harmonic
} WaveformResult;

/* Memory calculation macro */
#define DSP_CALC_WORK_BUF_SIZE(ch_num, points) \
    ( ((ch_num) * (points) * sizeof(uint32_t)) + \
      ((ch_num) * sizeof(uint32_t)) + \
      ((ch_num) * 48) + \
      ((ch_num) * sizeof(float)) + \
      ((ch_num) * sizeof(uint8_t)) )

/* --- Engine Lifecycle Management --- */

// Inject parameters to library
uint8_t DSP_Algo_Init(uint8_t ch_num, uint16_t points_per_period, uint16_t settle_periods, 
                      uint32_t sample_freq_hz, uint8_t *work_buf, uint32_t buf_size, 
                      uint8_t (*ch_enable_cb)(uint8_t));

MonitorResult Calc_CirculatingCurrent(uint16_t *data, uint16_t len, float k);

/* --- Data Stream Interface --- */
uint8_t Calc_Accumulate_WithAlign(uint8_t ch, uint16_t *data, int16_t *offset_out);

/* --- Data Settlement Interface --- */
// Settle data every 1s

void Calc_GetFullWaveform(uint8_t ch, float k, int16_t last_shift, uint16_t actual_cycles, WaveformResult* out_res);
/* --- Vector Sum Engine --- */
// 1. Accumulate vector sum
void Calc_Accumulate_VectorSum(uint16_t *interleaved_data);

// 2. Vector sum settlement
float Calc_GetVectorSumRMS(uint8_t group, float k, uint16_t actual_cycles);

#endif
