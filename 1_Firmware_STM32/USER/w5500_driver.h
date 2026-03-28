#ifndef __W5500_DRIVER_H
#define __W5500_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>
#include "sys_config.h" // Include sys_config for TCP_PORT and MB_REG definitions

/**
 * Hardware Distribution Specification:
 * 1. Communication Pins: All use GPIOB (PB12-PB15, PB0) to avoid PA0-PA7 sampling area.
 * 2. Sampling Pins: PA0-PA3 handle ADC current sampling without interference.
 */

// --- 1. W5500 Pin Definitions on GPIOB ---
#define W5500_SCS_PIN      GPIO_Pin_12   // PB12: SPI Chip Select (SCS/NSS)
#define W5500_SCLK_PIN     GPIO_Pin_13   // PB13: SPI Clock (SCK/SCLK)
#define W5500_MISO_PIN     GPIO_Pin_14   // PB14: Master In Slave Out (MISO)
#define W5500_MOSI_PIN     GPIO_Pin_15   // PB15: Master Out Slave In (MOSI)
#define W5500_RST_PIN      GPIO_Pin_0    // PB0: Hardware Reset Pin (RST)
#define W5500_INT_PIN      GPIO_Pin_1    // PB1: Interrupt Pin (INT) - Reserved

// --- 2. Software SPI Fast Operation Macros (For GPIOB) ---
#define W5500_SCS(n)  ((n)?GPIO_SetBits(GPIOB, W5500_SCS_PIN):GPIO_ResetBits(GPIOB, W5500_SCS_PIN))
#define W5500_SCLK(n) ((n)?GPIO_SetBits(GPIOB, W5500_SCLK_PIN):GPIO_ResetBits(GPIOB, W5500_SCLK_PIN))
#define W5500_MOSI(n) ((n)?GPIO_SetBits(GPIOB, W5500_MOSI_PIN):GPIO_ResetBits(GPIOB, W5500_MOSI_PIN))
#define W5500_MISO    GPIO_ReadInputDataBit(GPIOB, W5500_MISO_PIN)

// --- 3. Core Function Interfaces (Decoupled Design) ---

/**
 * @brief Initialize GPIOB hardware pins and perform W5500 hardware reset
 * Ensure executed after or parallel to ADC_Init to avoid interfering with PA group sampling.
 */
void W5500_GPIO_Init(void);

/**
 * @brief Network Parameter Synchronization
 * Synchronize IP/Mask/Gateway from g_modbus_regs[11-22].
 * Fallback to 192.168.1.100 if data is invalid.
 */
void W5500_Sync_Params(uint8_t *ip, uint8_t *sn, uint8_t *gw, uint16_t port);

/**
 * @brief Open Socket Service
 * Start listening using TCP_PORT (default 502) defined in sys_config.h.
 */
void W5500_Socket_Open(void);

/**
 * @brief W5500 Polling Process Function
 * 1. Handle received large packets (up to 1100 bytes).
 * 2. Pass to dispatcher, supporting 1-100 configuration writes and 101+ read-only acquisitions.
 */
void W5500_Process(void);

/**
 * @brief Physical Layer Send Wrapper
 * Used to respond to Modbus TCP requests or actively send 0xAA waveform transparent data.
 */
void W5500_Send_Packet(uint8_t *buf, uint16_t len);

#endif /* __W5500_DRIVER_H */
