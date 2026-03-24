#include "w5500_driver.h"
#include "protocol_dispatcher.h"
#include "sys_tick.h"
#include "delay.h"
#include <string.h>

#include "wizchip_conf.h"
#include "socket.h"


/**
 * [Hardware Mapping]
 * 1. Register 1-100 Write
 * 2. Register 101-138
 * 3. Comm pins: GPIOB (PB12-PB15, PB0), ADC pins: PA
 */

static uint8_t g_rx_buf[5000]; // Support transparent transmission of 0xAA large packets

/* --- 1. SPI Interface --- */
/**
 * [Low-level SPI Read/Write]
 * Register to WIZnet library. Using Software SPI.
 */
uint8_t W5500_ReadWriteByte(uint8_t dat) {
    uint8_t i;
    for(i=0; i<8; i++) {
        W5500_SCLK(0); 
        if(dat & 0x80) W5500_MOSI(1);
        else W5500_MOSI(0);
        dat <<= 1;
        W5500_SCLK(1); // SCK
        if(W5500_MISO) dat |= 0x01;
        W5500_SCLK(0);
    }
    return dat;
}

/* WIZnet library callback functions */
void wizchip_select(void) {
    W5500_SCS(0);
}
void wizchip_deselect(void) {
    W5500_SCS(1);
}
uint8_t wizchip_read(void) {
    return W5500_ReadWriteByte(0xFF);
}
void wizchip_write(uint8_t wb) {
    W5500_ReadWriteByte(wb);
}

/* --- 2. GPIO Init --- */
void W5500_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // Configure PB12, PB13, PB15, PB0 as output
    GPIO_InitStructure.GPIO_Pin = W5500_SCS_PIN | W5500_SCLK_PIN | W5500_MOSI_PIN | W5500_RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure PB14 as input
    GPIO_InitStructure.GPIO_Pin = W5500_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    W5500_SCS(1); 
    
    GPIO_ResetBits(GPIOB, W5500_RST_PIN);
    Wait_Ms(10);
    GPIO_SetBits(GPIOB, W5500_RST_PIN);
    Wait_Ms(50);

    // Register SPI callbacks
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    
    // Init RX/TX buffer size
    uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
    ctlwizchip(CW_INIT_WIZCHIP, (void*)memsize);
}

/* --- 3. Network Configuration --- */
// Note: Ensure header declaration matches.
void W5500_Sync_Params(uint8_t *ip, uint8_t *sn, uint8_t *gw, uint16_t port) {
    uint8_t def_ip[4] = {192, 168, 1, 100};
    uint8_t def_gw[4] = {192, 168, 1, 1};
    uint8_t def_sn[4] = {255, 255, 255, 0};

    // Set defaults if IP is invalid
    if(ip[0] == 0 || ip[0] == 255) {
        memcpy(ip, def_ip, 4);
        memcpy(gw, def_gw, 4);
        memcpy(sn, def_sn, 4);
    }

    // Set net info via official struct
    wiz_NetInfo net_info = {
        .mac = {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33}, // Default MAC
        .ip = {ip[0], ip[1], ip[2], ip[3]},
        .sn = {sn[0], sn[1], sn[2], sn[3]},
        .gw = {gw[0], gw[1], gw[2], gw[3]},
        .dns = {8, 8, 8, 8},
        .dhcp = NETINFO_STATIC
    };
    wizchip_setnetinfo(&net_info);
}

/* --- 4. Socket Process --- */
/**
 * [Refactor]: Robust TCP Server state machine.
 */
void W5500_Process(void) {
    uint8_t sn = 0; // Bind to Socket 0
    int32_t ret;
    uint16_t size = 0;
    
    switch(getSn_SR(sn)) {
        case SOCK_ESTABLISHED: // Connection established
            if(getSn_IR(sn) & Sn_IR_CON) {
                setSn_IR(sn, Sn_IR_CON); // Clear interrupt flag
            }
            
            // Check received data
            size = getSn_RX_RSR(sn);
            if(size > 0) {
                if(size > 5000) size = 5000;
                
                // Read data
                ret = recv(sn, g_rx_buf, size);
                if(ret > 0) {
                    // Route data to protocol dispatcher
                    Protocol_Dispatch(g_rx_buf, ret, SOURCE_W5500);
                }
            }
            break;
            
        case SOCK_CLOSE_WAIT: // Client disconnected
            disconnect(sn);
            break;
            
        case SOCK_INIT: // Listen socket
            listen(sn);
            break;
            
        case SOCK_CLOSED: // Open socket
            socket(sn, Sn_MR_TCP, 502, 0); // Use macro for port
            break;
            
        default:
            break;
    }
}

void W5500_Send_Packet(uint8_t *buf, uint16_t len) {
    // Advanced send function
    send(0, buf, len); 
}

/**
 * @brief Open Socket
 * Listen on TCP_PORT.
 */
void W5500_Socket_Open(void) {
    // Listen logic is handled in W5500_Process state machine.
    // Keep empty.
}
