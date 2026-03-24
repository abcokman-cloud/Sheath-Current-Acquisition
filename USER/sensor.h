#ifndef USER_SENSOR_H_
#define USER_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#include "sys_config.h" // Include global configurations

// Sensor channel enumeration
typedef enum {
    SENSOR_CH_OSC_TEMP = 0, // Oscillator temp channel
    // Add other channels here...
} SensorChannel_e;

/**
 * @brief Core Interface: Get Offset
 *        Internally uses config thresholds
 * @param ch Sensor channel
 * @return Calculated physical offset
 */
float Sensor_GetDspOffset(uint8_t ch);

/* --- Hardware Pin Allocation --- */
#define SENSOR_GPIO_PORT    GPIOC               // Port C
#define SENSOR_GPIO_PIN     GPIO_Pin_5          // Pin PC5
#define SENSOR_ADC_CH       ADC_Channel_15      // ADC1 Channel 15

/* --- Function Declarations --- */
void Sensor_Init(void);         // Init PC5 GPIO
void Sensor_Update_Task(void);  // 1Hz task triggered by main


#ifdef __cplusplus
}
#endif

#endif /* USER_SENSOR_H_ */
