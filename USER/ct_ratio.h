#ifndef __CT_RATIO_H
#define __CT_RATIO_H

#include "stm32f4xx.h"
#include "dsp_algo.h"  // Requires WaveformResult

// 1. Base Configuration

// 2. External Registers
// Modbus register pool
extern uint16_t g_modbus_regs[]; 

// Deprecated, using g_waveform_data

// 4. Interfaces
/**
 * @brief Process and distribute data
 * @param ch Channel index (0-7)
 * @param w_res DSP result package
 * @param last_shift Phase alignment offset
 */
void Final_Deep_Processing(uint8_t ch, WaveformResult* w_res, int16_t last_shift);

#endif
