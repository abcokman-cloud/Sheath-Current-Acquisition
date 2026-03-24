#include "modbus_slave.h"
#include "sys_config.h"
#include <string.h>

/**
 * @brief Standard CRC16 calculation.
 */
uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (uint8_t i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief Modbus parsing and execution engine.
 * @note Supports 0x03, 0x06, 0x10. Multiplexed for UART/Ethernet.
 */
uint16_t Modbus_Parse_And_Execute(uint8_t *pIn, uint16_t inLen, uint8_t *pOut, bool useCRC) {
    // 1. Minimum length check
    if (inLen < 4) return 0;

    // 2. Check CRC if UART mode
    if (useCRC) {
        uint16_t crc_rcvd = (pIn[inLen - 1] << 8) | pIn[inLen - 2];
        if (Modbus_CRC16(pIn, inLen - 2) != crc_rcvd) return 0;
    }

    // 3. Device ID check
    if (pIn[0] != g_modbus_regs[MB_REG_DEV_ID] && pIn[0] != 0x01) return 0; // Diagnostic patch: force allow ID 0x01

    uint8_t f_code = pIn[1];
    uint16_t start_addr = (pIn[2] << 8) | pIn[3];
    uint16_t quantity = (pIn[4] << 8) | pIn[5];
    uint16_t res_len = 0;

    // --- Function 0x03: Read Holding Registers ---
    if (f_code == 0x03) {
        if (start_addr + quantity > 512) return 0;

        pOut[0] = pIn[0];
        pOut[1] = 0x03;
        pOut[2] = (uint8_t)(quantity * 2);

        for (uint16_t i = 0; i < quantity; i++) {
            uint16_t val = g_modbus_regs[start_addr + i]; // Read from memory pool
            pOut[3 + i * 2] = (uint8_t)(val >> 8);
            pOut[4 + i * 2] = (uint8_t)(val & 0xFF);
        }
        res_len = 3 + quantity * 2;
    }
    // --- Function 0x06: Write Single Register ---
    else if (f_code == 0x06) {
        uint16_t write_val = quantity; // Extract write value
        
        // Security barrier: limit writes to config area (1-100)
        if (start_addr >= 1 && start_addr <= 100) {
            g_modbus_regs[start_addr] = write_val; // Write value
            memcpy(pOut, pIn, 6);
            res_len = 6;
        }
    }
    // --- Function 0x10: Write Multiple Registers ---
    else if (f_code == 0x10) {
        // Security barrier: restrict write boundaries
        if (start_addr >= 1 && (start_addr + quantity - 1) <= 100) {
            for (uint16_t i = 0; i < quantity; i++) {
                g_modbus_regs[start_addr + i] = (pIn[7 + i * 2] << 8) | pIn[8 + i * 2];
            }
            memcpy(pOut, pIn, 6);
            res_len = 6;
        }
    }

    // 4. Append CRC if UART mode
    if (useCRC && res_len > 0) {
        uint16_t crc = Modbus_CRC16(pOut, res_len);
        pOut[res_len++] = (uint8_t)(crc & 0xFF);
        pOut[res_len++] = (uint8_t)(crc >> 8);
    }

    return res_len;
}
