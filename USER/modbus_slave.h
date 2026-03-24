#ifndef __MODBUS_SLAVE_H
#define __MODBUS_SLAVE_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>

/* --- Modbus Exception Codes (For error response) --- */
#define MB_ERR_NONE             0x00
#define MB_ERR_FUNC_INVALID     0x01    // Invalid function code
#define MB_ERR_ADDR_INVALID     0x02    // Invalid data address
#define MB_ERR_VALUE_INVALID    0x03    // Invalid data value
#define MB_ERR_READ_ONLY        0x04    // Read only error

/* ==================================================================
 * Core Interface: Parsing engine called by protocol dispatcher
 * ================================================================== */

/**
 * @brief Modbus core parsing and execution function
 * @param pIn    Pointer to the received raw packet (RTU packet or payload without TCP header)
 * @param inLen  Length of the received packet
 * @param pOut   Buffer to store the response packet
 * @param useCRC Whether to execute/generate CRC check (true for UART mode, false for TCP mode)
 * @return uint16_t Total bytes of the response packet, 0 means no response
 */
uint16_t Modbus_Parse_And_Execute(uint8_t *pIn, uint16_t inLen, uint8_t *pOut, bool useCRC);

/* ==================================================================
 * Low-level utility functions and compatibility interfaces
 * ================================================================== */

/**
 * @brief Standard CRC16 calculation
 * @param buf Pointer to data buffer
 * @param len Length of data to calculate
 * @return uint16_t Calculated CRC16 value
 */
uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len);

#endif /* __MODBUS_SLAVE_H */
