#include "usart.h"
#include "protocol_dispatcher.h"
#include "led.h"

/*
 * Buffer size expanded to 1100 to support reading 512 registers.
 * Uses IDLE interrupt for frame break, asynchronous processed by dispatcher via 0x8000 flag.
 */

#if EN_USART1_RX   
uint8_t  USART_RX_BUF[USART_REC_LEN];     // Receive buffer
uint16_t USART_RX_STA = 0;                // Receive status/length counter

/**
 * @brief Support printf redirection
 */
#if 1
#pragma import(__use_no_semihosting)             
struct __FILE { int handle; }; 
FILE __stdout;       
void _sys_exit(int x) { x = x; } 
int fputc(int ch, FILE *f) { 	
    while((USART1->SR & 0X40) == 0); 
    USART1->DR = (uint8_t)ch;      
    return ch;
}
#endif

/**
 * @brief Low-level UART data send function
 * Called by modbus_slave.c or other protocol modules
 */
void UART_Send_Data(uint8_t *buf, uint16_t len) {
    RS485_TX_EN(); // Switch to TX mode before sending
    
    for(uint16_t i = 0; i < len; i++) {
        while((USART1->SR & 0X40) == 0); 
        USART1->DR = buf[i];
    }
    while((USART1->SR & 0X40) == 0); // Wait for the last byte to leave the shift register
    RS485_RX_EN(); // Switch back to RX mode after sending
}

/**
 * @brief USART1 Initialization (PA9-TX, PA10-RX)
 * @param bound Baud rate
 */
void uart_init(uint32_t bound) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. Enable Clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); 
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); 

    // 2. Pin AF mapping
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1); 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1); 
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; 
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 
    GPIO_Init(GPIOA, &GPIO_InitStructure); 

    // 3. Serial port configuration
    USART_InitStructure.USART_BaudRate = bound; 
    USART_InitStructure.USART_WordLength = USART_WordLength_8b; 
    USART_InitStructure.USART_StopBits = USART_StopBits_1; 
    USART_InitStructure.USART_Parity = USART_Parity_No; 
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; 
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	
    USART_Init(USART1, &USART_InitStructure); 
    
    // RS485 direction control pin initialization
    GPIO_InitStructure.GPIO_Pin = RS485_RE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(RS485_RE_PORT, &GPIO_InitStructure);
    RS485_RX_EN(); // Default to receive mode

    // 4. Enable receive and idle interrupts
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); 

    USART_Cmd(USART1, ENABLE);  
    
    // 5. Interrupt priority configuration
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
    NVIC_Init(&NVIC_InitStructure);	
}

/**
 * @brief USART1 Interrupt Service Routine
 * Uses IDLE flag to determine Modbus frame end, and set 0x8000 bit.
 */
void USART1_IRQHandler(void) {
    uint8_t res;
    
    // --- Byte reception logic (RXNE) ---
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        res = USART_ReceiveData(USART1);
        
        // Toggle LED when data received
        LED_Toggle();
        
        // Only receive new data if old data has been processed (0x8000 is clear)
        if((USART_RX_STA & 0x8000) == 0) {
            // Defensive check: leave 2 bytes of space to prevent overflow
            if(USART_RX_STA < (USART_REC_LEN - 2)) {
                USART_RX_BUF[USART_RX_STA++] = res; 
            } else {
                USART_RX_STA = 0; // Force reset to avoid long packet interference
            }
        }
    }
    
    // --- Frame end detection (IDLE) ---
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        // Standard sequence to clear IDLE hardware flag
        res = USART1->SR; 
        res = USART1->DR; 
        
        if(USART_RX_STA > 0) {
            // Set highest bit to notify Protocol_Dispatch_Center
            USART_RX_STA |= 0x8000; 
        }
    }
}
#endif
