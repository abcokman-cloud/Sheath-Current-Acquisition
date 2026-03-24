#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include <stdio.h>

/* --- USART RX Buffer Configuration --- */
// Maximum receive bytes
#define USART_REC_LEN  2048  

// Receive status flag
// bit15: Receive complete flag (1: Complete, 0: Incomplete)
// bit14~0: Number of valid bytes received
extern uint16_t USART_RX_STA; 
extern uint8_t  USART_RX_BUF[USART_REC_LEN]; 

/* --- RS485 Direction Control Pin Definition --- */
#define RS485_RE_PORT       GPIOA
#define RS485_RE_PIN        GPIO_Pin_8
#define RS485_TX_EN()       GPIO_SetBits(RS485_RE_PORT, RS485_RE_PIN)
#define RS485_RX_EN()       GPIO_ResetBits(RS485_RE_PORT, RS485_RE_PIN)

/* --- Core Functions --- */
// USART1 initialization
void uart_init(uint32_t bound);

// USART fast send function
void UART_Send_Data(uint8_t *buf, uint16_t len);

#endif /* __USART_H */
