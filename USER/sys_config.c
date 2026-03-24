/**
 * ==================================================================
 * File: sys_config.c
 * Description: System global configuration management, Flash IO, Modbus register mapping
 * ==================================================================
 */

#include <stdint.h>
// Core includes
#include "sys.h"        
#include "sys_config.h" 
#include <string.h>      // memset support

/* ================================================================== */
/* Global Variable Definition                                         */
/* ================================================================== */

// Global Modbus register pool
uint16_t g_modbus_regs[512] = {0};

// System running status variables
uint8_t  g_sys_xtal_en    = 0;
float    g_sys_now_temp   = 25.0f;
float    g_sys_xtal_drift = 0.0f;

// Flash storage configuration
#define CONFIG_FLASH_ADDR  0x080E0000  // Flash Sector 11 address
#define CONFIG_MAGIC_NUM   0x5AA5      // Initialization magic number

/* ================================================================== */
/* Channel Enable Control                                             */
/* ================================================================== */

// Get specified channel enable status (1 or 0)
uint8_t SYS_Get_Ch_Enable(uint8_t ch) {
    if(ch >= CH_NUM) return 0;
    return (uint8_t)g_modbus_regs[MB_REG_CH_ENABLE_START + ch];
}

/* ================================================================== */
/* Modbus Data Push Functions                                         */
/* ================================================================== */
void SYS_Push_Float_To_Modbus(uint16_t start_addr, uint8_t ch, float val_f) {
    if (ch >= CH_NUM) return;

    // Convert float to int with x100 scale
    uint32_t val_u32 = (uint32_t)(val_f * 100.0f);
    uint16_t base_idx = start_addr + (ch * 2);

    // Store as two 16-bit words (Big-Endian)
    g_modbus_regs[base_idx]     = (uint16_t)(val_u32 >> 16);    // High 16 bits
    g_modbus_regs[base_idx + 1] = (uint16_t)(val_u32 & 0xFFFF); // Low 16 bits
}

// Push IEEE 754 float value directly to Modbus registers
void SYS_Push_IEEE_Float_To_Modbus(uint16_t start_addr, uint8_t ch, float val_f) {
    if (ch >= CH_NUM) return;
    uint16_t base_idx = start_addr + (ch * 2);
    
    // Float to integer memory mapping via union
    union {
        float f;
        uint32_t u32;
    } data;
    data.f = val_f;
    
    g_modbus_regs[base_idx]     = (uint16_t)(data.u32 >> 16);
    g_modbus_regs[base_idx + 1] = (uint16_t)(data.u32 & 0xFFFF);
}

/* ================================================================== */
/* Flash Operations                                                   */
/* ================================================================== */
// Save config to Flash
void SYS_Save_Config(void) {
    uint32_t i;
    
    // 1. Unlock Flash
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }

    // 2. Clear error flags
    FLASH->SR = 0xF3; 

    // 3. Erase Sector 11
    FLASH->CR &= ~((uint32_t)0x00000078); 
    FLASH->CR |= (11 << 3);               
    FLASH->CR |= FLASH_CR_SER;            
    FLASH->CR |= FLASH_CR_STRT;           
    while (FLASH->SR & FLASH_SR_BSY);     

    // 4. Enable programming mode
    FLASH->CR &= ~FLASH_CR_SER;
    FLASH->CR |= FLASH_CR_PG;

    // 5. Set parallelism
    FLASH->CR &= ~FLASH_CR_PSIZE;         
    FLASH->CR |= FLASH_CR_PSIZE_0;        

    // 6. Write magic number
    g_modbus_regs[0] = CONFIG_MAGIC_NUM; 

    // 7. Write Modbus registers (0 - 127)
    for(i = 0; i < 128; i++) {
        *(__IO uint16_t*)(CONFIG_FLASH_ADDR + (i * 2)) = g_modbus_regs[i];
        while (FLASH->SR & FLASH_SR_BSY); 
    }

    // 8. Lock Flash
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->CR |= FLASH_CR_LOCK;
}

/* ================================================================== */
/* Restore Factory Settings                                           */
/* ================================================================== */
void SYS_RestoreToFactory(void) {
    uint8_t i;
    uint8_t def_ip[4]   = DEFAULT_IP;
    uint8_t def_mask[4] = DEFAULT_MASK;
    uint8_t def_gw[4]   = DEFAULT_GW;

    // Clear register pool completely
    memset(g_modbus_regs, 0, sizeof(g_modbus_regs));

    // 1. Restore comm parameters
    g_modbus_regs[MB_REG_DEV_ID]     = DEFAULT_MB_ID; 
    g_modbus_regs[MB_REG_BAUDRATE_H] = (uint16_t)(DEFAULT_BAUDRATE >> 16);
    g_modbus_regs[MB_REG_BAUDRATE_L] = (uint16_t)(DEFAULT_BAUDRATE & 0xFFFF);

    // 2. Restore network parameters
    for(i = 0; i < 4; i++) {
        g_modbus_regs[MB_REG_IP_START + i]   = def_ip[i];
        g_modbus_regs[MB_REG_MASK_START + i] = def_mask[i];
        g_modbus_regs[MB_REG_GW_START + i]   = def_gw[i];
    }

    // 3. Restore channel ratios and offsets
    for(i = 0; i < CH_NUM; i++) {
        SYS_Push_IEEE_Float_To_Modbus(MB_REG_RATIO_START, i, 1.0f);  // Default ratio
        SYS_Push_IEEE_Float_To_Modbus(MB_REG_OFFSET_START, i, 0.0f); // Default offset
    }

    // 4. Restore system status parameters
    g_modbus_regs[MB_REG_SYS_XTAL_EN]    = DEFAULT_XTAL_EN_STATE;
    g_modbus_regs[MB_REG_SYS_TEMP_VALUE] = (uint16_t)(DEFAULT_SYS_TEMP * 10.0f);
    g_modbus_regs[MB_REG_SYS_XTAL_DRIFT] = (uint16_t)(DEFAULT_XTAL_DRIFT);

    // 5. Restore channel enable defaults
    for(i = 0; i < 4; i++) g_modbus_regs[MB_REG_CH_ENABLE_START + i] = 1;
    for(i = 4; i < 8; i++) g_modbus_regs[MB_REG_CH_ENABLE_START + i] = 0;

    // Reset commands
    g_modbus_regs[MB_REG_SAVE_CMD]    = DEFAULT_SAVE_CMD;     
    g_modbus_regs[MB_REG_RESTORE_CMD] = DEFAULT_RESTORE_CMD;  

    // 7. Save to Flash
    SYS_Save_Config();
}

/* ================================================================== */
/* System Initialization                                              */
/* ================================================================== */
void SYS_Init(void) {
    uint8_t i;

    // 1. Read configs from Flash
    for(i = 0; i < 128; i++) {
        g_modbus_regs[i] = *(__IO uint16_t*)(CONFIG_FLASH_ADDR + i*2);
        
        // Clean 0xFFFF values
        if (g_modbus_regs[i] == 0xFFFF) {
            g_modbus_regs[i] = 0;
        }
    }

    // 2. Verify magic number; restore if mismatched
    if (g_modbus_regs[0] != CONFIG_MAGIC_NUM) {
        SYS_RestoreToFactory();
    }

    // 3. Initialize global state variables
    g_sys_xtal_en = (uint8_t)g_modbus_regs[MB_REG_SYS_XTAL_EN];
    g_sys_now_temp = (float)g_modbus_regs[MB_REG_SYS_TEMP_VALUE] / 10.0f;
}

/* ================================================================== */
/* Command Monitor (Called in main loop)                              */
/* ================================================================== */
void SYS_Monitor_Cmd(void) {
    // 1. Check save command
    if (g_modbus_regs[MB_REG_SAVE_CMD] == 1) { 
        SYS_Save_Config();                  
        g_modbus_regs[MB_REG_SAVE_CMD] = DEFAULT_SAVE_CMD; 
    }

    // 2. Check restore factory command
    if (g_modbus_regs[MB_REG_RESTORE_CMD] == 1) {
        SYS_RestoreToFactory();             
        g_modbus_regs[MB_REG_RESTORE_CMD] = DEFAULT_RESTORE_CMD; 
    }
}

/* ================================================================== */
/* Parameter Accessors                                                */
/* ================================================================== */
float SYS_Get_CT_Ratio(uint8_t ch) {
    if(ch >= CH_NUM) return 1.0f;
    union { float f; uint32_t u32; } data;
    
    uint16_t base_idx = MB_REG_RATIO_START + (ch * 2);
    data.u32 = ((uint32_t)g_modbus_regs[base_idx + 1] << 16) | g_modbus_regs[base_idx];
    
    return data.f;
}

float SYS_Get_CT_Offset(uint8_t ch) {
    if(ch >= CH_NUM) return 0.0f;
    union { float f; uint32_t u32; } data;
    
    uint16_t base_idx = MB_REG_OFFSET_START + (ch * 2);
    data.u32 = ((uint32_t)g_modbus_regs[base_idx + 1] << 16) | g_modbus_regs[base_idx];
    
    return data.f;
}
/* Global communication buffers */
uint8_t response_buf[5000]; // Response buffer for Ethernet
uint16_t g_waveform_data[CH_NUM][POINTS_PER_PERIOD]; // Store extracted waveform data
