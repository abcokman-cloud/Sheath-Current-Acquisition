/**
  ******************************************************************************
  * @file    sys_tick.c
  * @brief   System SysTick and timebase service.
  * @note    1ms timestamp, precise delay, and clock enablers.
  ******************************************************************************
  */

#include "sys_tick.h"
#include "led.h" 

/* --- Timestamp Variables --- */
// 1ms absolute timestamp
volatile uint32_t g_sys_tick = 0; 

/**
 * @brief System hardware init (Clocks)
 * @note  Enable all required peripheral clocks.
 */
void SysTick_Timer_Init(void)
{
    /* 1. Enable peripheral clocks */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | 
                           RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_USART1, ENABLE);

    /* 2. Configure SysTick for 1ms interrupt */
    SysTick_Config(SystemCoreClock / 1000); 
}

/**
 * @brief Get current tick in ms.
 * @return uint32_t ms since boot.
 */
uint32_t Get_System_Time(void) 
{
    return g_sys_tick;
}

/**
 * @brief Blocking ms delay.
 * @param ms milliseconds to delay
 * @note  Only for hardware init. Do not use in main loop.
 */
void Wait_Ms(uint32_t ms) 
{
    uint32_t start = Get_System_Time();
    while((Get_System_Time() - start) < ms);
}

/* 
 * SysTick_Handler is implemented in stm32f4xx_it.c
 */
