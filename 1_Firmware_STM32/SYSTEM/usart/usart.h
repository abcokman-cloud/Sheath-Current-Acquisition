#ifndef __USART_H
#define __USART_H

#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

/**
 * Buffer size expanded to 1100 to support reading 512 registers.
 * Uses IDLE interrupt for frame break, asynchronous processed by dispatcher via 0x8000 flag.
 */

// Max receive buffer length
// 512 registers = 1024 bytes + Modbus overhead, 1100 is a safe boundary
#define USART_REC_LEN  			1100  	
#define EN_USART1_RX 			1		// Enable USART1 receiver

/* --- RS485 direction control pin definition --- */
#define RS485_RE_PORT       GPIOA
#define RS485_RE_PIN        GPIO_Pin_8
#define RS485_TX_EN()       GPIO_SetBits(RS485_RE_PORT, RS485_RE_PIN)
#define RS485_RX_EN()       GPIO_ResetBits(RS485_RE_PORT, RS485_RE_PIN)

/**
 * @brief UART Global variable declarations
 * USART_RX_STA logic:
 * bit15:      Receive complete flag (Set to 1 by IDLE interrupt)
 * bit14~0:    Number of valid bytes received
 */
extern uint8_t  USART_RX_BUF[USART_REC_LEN]; // Receive buffer
extern uint16_t USART_RX_STA;         		 // Receive status flag	

/**
 * @brief USART1 Initialization
 * @param bound Baud rate (e.g. 115200)
 */
void uart_init(uint32_t bound);

/**
 * @brief Low-level UART data send function
 * Called by modbus_slave.c or protocol_dispatcher.c
 * @param buf Data pointer
 * @param len Data length
 */
void UART_Send_Data(uint8_t *buf, uint16_t len);

#endif
