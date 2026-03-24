#include "sensor.h"
#include "sys.h"
#include "sys_config.h" 
#include "stm32f4xx.h"
#include "sys_tick.h"

// PC4 corresponds to ADC CH14
#define XTAL_TEMP_ADC_CH  ADC_Channel_14

// Last update timestamp
static uint32_t last_xtal_update_tick = 0; 

// Oscillator temperature drift variable
static float g_sensor_xtal_drift[CH_NUM] = {0.0f}; 

/* Core logic note */

/* Init Sensor */
void Sensor_Init(void) {
    // 1. Enable GPIOC clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // 2. Configure PC4 as Analog IN
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Reset timestamp
    last_xtal_update_tick = 0; 

    // Load default drift value
    for(uint8_t i = 0; i < CH_NUM; i++) {
        g_sensor_xtal_drift[i] = DEFAULT_XTAL_DRIFT;
    }
}

/* Sensor Task (1Hz) */
void Sensor_Update_Task(void) {
    // --- [1Hz Frequency Control] ---
    // Exit if less than 1000ms
    uint32_t current_tick = Get_System_Time();
    if (current_tick - last_xtal_update_tick < 1000) {
        return; 
    }
    last_xtal_update_tick = current_tick; 

    uint16_t adc_raw = 0;

    // --- 1. Switch ADC to PC4 ---
    // Use 480 cycles for stable slow read
    ADC_RegularChannelConfig(ADC1, XTAL_TEMP_ADC_CH, 1, ADC_SampleTime_480Cycles);
    
    // --- 2. Start Conversion ---
    ADC_SoftwareStartConv(ADC1);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); // Short wait allowed for 1Hz task
    
    // --- 3. Fetch data ---
    adc_raw = ADC_GetConversionValue(ADC1);
    g_modbus_regs[MB_REG_SYS_TEMP_VALUE] = adc_raw;

    // Get temperature for float calculations
    float current_temp = (float)adc_raw;
    
    // Execute only if drift compensation is enabled
    if (g_modbus_regs[MB_REG_SYS_XTAL_EN] != 0) {
        /* Calculate drift based on PC5 */
        // Example formula
        g_sensor_xtal_drift[0] = (current_temp - 2048.0f) * 0.0001f;
    } else {
        g_sensor_xtal_drift[0] = 0.0f;
    }

    // Update global register
    g_modbus_regs[MB_REG_SYS_XTAL_DRIFT] = (uint16_t)(g_sensor_xtal_drift[0] * 100);

    // --- 4. Restore Default ADC Channel ---
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_15Cycles);
}
