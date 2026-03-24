#include "led.h" 

/* LED Driver for STM32F407 */

/* Initialize LED IO */
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // Enable GPIOA clock

  // Configure PA0 for LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;           // LED IO pin
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;       // Standard output mode
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // Push-pull output
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 100MHz speed
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // Pull-up resistor
  GPIO_Init(GPIOA, &GPIO_InitStructure);              // Initialize GPIO
	
  GPIO_SetBits(GPIOA, GPIO_Pin_0);                    // Set PA0 high to turn off LED
}

/**
 * @brief Toggle LED pin status (PA0)
 */
void LED_Toggle(void)
{
    GPIOA->ODR ^= GPIO_Pin_0; 
}
