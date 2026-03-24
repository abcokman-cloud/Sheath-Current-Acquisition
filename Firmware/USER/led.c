/**
  ******************************************************************************
  * @file    led.c
  * @brief   System status indicator LED module implementation
  ******************************************************************************
  */

#include "led.h"

/**
 * @brief Initialize LED hardware pin (Push-Pull Output)
 */
void LED_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. Enable Clock
    RCC_AHB1PeriphClockCmd(LED_GPIO_CLK, ENABLE);

    // 2. Configure pin attributes
    GPIO_InitStructure.GPIO_Pin = LED_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);

    // 3. Initial state OFF (assuming low is ON, high is OFF)
    GPIO_SetBits(LED_GPIO_PORT, LED_GPIO_PIN);
}

void LED_Toggle(void) {
    LED_GPIO_PORT->ODR ^= LED_GPIO_PIN; // Register XOR operation to toggle
}