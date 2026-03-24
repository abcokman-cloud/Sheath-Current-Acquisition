#ifndef __LED_H
#define __LED_H
#include "sys.h"

/* LED Driver for STM32F407 */

// LED Port Definition
#define LED PAout(0)	// LED output

void LED_Init(void);     // Initialize LED GPIO
void LED_Toggle(void);   // Toggle LED status
#endif
