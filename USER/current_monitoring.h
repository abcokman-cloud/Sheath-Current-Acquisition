#ifndef __CURRENT_MONITORING_H
#define __CURRENT_MONITORING_H

#include <stdint.h>
#include "stm32f4xx.h"
#include "dsp_algo.h"
#include "sys_config.h" 

/* ================================================================== */
/* 4. Memory and State Variables                                      */
/* ================================================================== */

// DMA Ping-pong buffers
extern uint16_t ADC_CycleBuffer[2][TOTAL_DMA_SIZE];

// State machine flag
extern volatile uint8_t latest_ready_idx;

/* ================================================================== */
/* 5. Core Interface Declarations                                     */
/* ================================================================== */

/**
 * @brief Hardware init configuration.
 * Configures TIM2 for 12800Hz and DMA ping-pong mode.
 */
void ADC_DMA_Config(void);

/**
 * @brief 20ms periodic scheduling task.
 * Responsibilities:
 * 1. Check channel status
 * 2. Extract 256 points
 * 3. Update DSP accumulators
 * 4. Drive DSP algorithm
 * 5. Finalize features every 50 cycles
 */
void Process_Cycle_Task(void);

#endif /* __CURRENT_MONITORING_H */
