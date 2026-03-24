/**
  ******************************************************************************
  * @file    main.c
  * @brief   Main entry point.
  * @note    Peripherals init and Super Loop scheduling.
  ******************************************************************************
  */

// --- Hardware & Drivers ---
#include "stm32f4xx.h"
#include <string.h>        // For memset
#include "sys_tick.h"      // 1ms SysTick
#include "usart.h"         // USART Driver
#include "led.h"           // LED Control

// --- Application Layer ---
#include "sys_config.h"          // Config & Flash Storage
#include "current_monitoring.h"  // Sampling & DSP Scheduling
#include "sensor.h"              // Slow Sensor ADC
#include "w5500_driver.h"        // W5500 Driver
#include "protocol_dispatcher.h" // Protocol Dispatcher

uint32_t last_tick = 0; // Timestamp for 500ms task

int main(void)
{
    // ==================================================================
    // 1. System Clock and SysTick Init
    // ==================================================================
    SysTick_Timer_Init(); 
    
    // ==================================================================
    // 2. Configuration Init
    // ==================================================================
    // Read parameters from Flash
    SYS_Init();

    // ==================================================================
    // 3. Basic Peripherals Init
    // ==================================================================
    LED_Init();           // LED init
    uart_init(115200);    // USART init
    
    // Ethernet init sequence
    W5500_GPIO_Init();
    
    // Supply network config to driver
    uint8_t e_ip[4], e_sn[4], e_gw[4];
    for(int i = 0; i < 4; i++) {
        e_ip[i] = (uint8_t)g_modbus_regs[MB_REG_IP_START + i];
        e_sn[i] = (uint8_t)g_modbus_regs[MB_REG_MASK_START + i];
        e_gw[i] = (uint8_t)g_modbus_regs[MB_REG_GW_START + i];
    }
    W5500_Sync_Params(e_ip, e_sn, e_gw, TCP_PORT); 
    
    W5500_Socket_Open();  // Open Socket 0
    
    Sensor_Init();        // Internal sensor init

    // ==================================================================
    // 4. Start High-Freq Sampling
    // ==================================================================
    // TIM2 triggers ADC + DMA Ping-Pong
    ADC_DMA_Config();     

    // ==================================================================
    // 5. Super Loop
    // ==================================================================
    while(1)
    {
        // [Task 1] DSP processing
        // Extract DMA buffers and run DSP
        Process_Cycle_Task(); 

        // [Task 2] Ethernet polling
        W5500_Process();

        // [Task 3] UART Modbus RTU processing
        if (USART_RX_STA & 0x8000) {  
            uint16_t rx_len = USART_RX_STA & 0x7FFF; 
            
            // Route to dispatcher
            bool is_ok = Protocol_Dispatch(USART_RX_BUF, rx_len, SOURCE_UART); 
            
            if (is_ok) {
                // Packet completed, clear buffer
                memset(USART_RX_BUF, 0, rx_len);
                USART_RX_STA = 0; 
            } else {
                // Incomplete packet, keep assembling
                // Clear flag only
                USART_RX_STA &= 0x7FFF; 
                
                // Defense against buffer overflow
                if ((USART_RX_STA & 0x7FFF) > (USART_REC_LEN - 100)) {
                    memset(USART_RX_BUF, 0, USART_REC_LEN);
                    USART_RX_STA = 0;
                }
            }
        }

        // [Task 4] 500ms Periodic Tasks
        if (Get_System_Time() - last_tick >= 500) { 
            last_tick = Get_System_Time();
            
            LED_Toggle();           // LED heartbeat
            Sensor_Update_Task();   // Sensor update
            SYS_Monitor_Cmd();      // Command monitor
        }
    }
}
