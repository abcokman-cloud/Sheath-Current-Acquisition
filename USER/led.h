/**
  ******************************************************************************
  * @file    led.h
  * @brief   System status LED module interface.
  ******************************************************************************
  */

#ifndef USER_LED_H_
#define USER_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

/* --- Hardware Pin Definitions --- */
#define LED_GPIO_PORT       GPIOA
#define LED_GPIO_PIN        GPIO_Pin_0
#define LED_GPIO_CLK        RCC_AHB1Periph_GPIOA

/* --- Interface Declarations --- */
void LED_Init(void);
void LED_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_LED_H_ */
