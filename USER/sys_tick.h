/**
  ******************************************************************************
  * @file    sys_tick.h
  * @brief   System timebase module
  ******************************************************************************
  */

#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include "sys.h"

/* --- Global Timestamp Declaration --- */
// Millisecond heartbeat that runs continuously after boot
extern volatile uint32_t g_sys_tick; 

/* --- Core Interface Declarations --- */
void SysTick_Timer_Init(void);      // Initialize SysTick timer and peripheral clocks
uint32_t Get_System_Time(void);     // Get current 1ms absolute timestamp
void Wait_Ms(uint32_t ms);          // Blocking ms delay (used for low-level hardware reset)

#endif
