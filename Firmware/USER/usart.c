#include "usart.h"
#include "led.h"

/* ================================================================== */
/* 1. Buffer and Flags                                                */
/* ================================================================== */
uint8_t  USART_RX_BUF[USART_REC_LEN];
uint16_t USART_RX_STA = 0; 

/* ================================================================== */
/* 2. printf Redirection                                              */
/* ================================================================== */
#if 1
#pragma import(__use_no_semihosting)             
struct __FILE { 
    int handle; 
}; 
FILE __stdout;       
void _sys_exit(int x) { 
    x = x; 
} 
void _ttywrch(int ch) {
    (void)ch;
}
int fputc(int ch, FILE *f) {      
    while((USART1->SR & 0X40) == 0); // Wait for previous TX to complete
    USART1->DR = (uint8_t) ch;      
    return ch;
}
#endif 

/* ================================================================== */
/* 3. Hardware Init (USART1)                                          */
/* ================================================================== */
void uart_init(uint32_t bound) {
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. Enable Clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // 2. Pin AF configuration
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    // 3. GPIO Configuration
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 4. USART Configuration
    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    // 5. Enable Interrupts (RXNE + IDLE)
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE); 
    
    // 6. NVIC Configuration
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // Priority lower than ADC DMA
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 7. RS485 DIR Pin Init
    GPIO_InitStructure.GPIO_Pin = RS485_RE_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(RS485_RE_PORT, &GPIO_InitStructure);
    RS485_RX_EN(); // Default to RX mode
    
    // 8. Enable Peripheral
    USART_Cmd(USART1, ENABLE);
}

/* ================================================================== */
/* 4. Low-level Transmit Interface                                    */
/* ================================================================== */
void UART_Send_Data(uint8_t *buf, uint16_t len) {
    RS485_TX_EN(); // Switch to TX mode
    
    for(uint16_t i = 0; i < len; i++) {
        // Check TXE during transmission
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, buf[i]);
    }
    // Ensure last byte is transmitted completely
    while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    
    RS485_RX_EN(); // Switch back to RX mode
}

/* ================================================================== */
/* 5. USART1 IRQ Handler                                              */
/* ================================================================== */
void USART1_IRQHandler(void) {
    uint32_t sr = USART1->SR; // 1. Read status register
    
    // 2. Handle errors
    // Clear error flags to prevent lockup
    if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        volatile uint32_t clear = USART1->DR; // Clear sequence: read SR, then read DR
        (void)clear;
    }

    // --- A. RXNE Interrupt ---
    if(sr & USART_SR_RXNE) {
        uint8_t res = USART1->DR; // Read DR to clear RXNE
        
        // Toggle LED on receive
        LED_Toggle();
        
        if((USART_RX_STA & 0x8000) == 0) { // If previous frame is processed
            USART_RX_BUF[USART_RX_STA & 0x7FFF] = res;
            if ((USART_RX_STA & 0x7FFF) < (USART_REC_LEN - 1)) {
                USART_RX_STA++;
            }
        }
    }
    
    // --- B. IDLE Interrupt ---
    if(sr & USART_SR_IDLE) {
        volatile uint32_t clear = USART1->DR; // Clear IDLE
        (void)clear;                // Avoid unused warning
        USART_RX_STA |= 0x8000;     // Set flag for main loop dispatcher
    }
}