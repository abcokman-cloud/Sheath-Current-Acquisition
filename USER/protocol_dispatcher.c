/**
  ******************************************************************************
  * @file    protocol_dispatcher.c
  * @brief   Protocol distribution and routing center.
  * @note    Responsibilities:
  *          1. Intercept 0xAA private high-speed protocol.
  *          2. Strip Modbus TCP/RTU frame and dispatch.
  *          3. Package response and route to hardware.
  ******************************************************************************
  */

#include "protocol_dispatcher.h"
#include "modbus_slave.h"
#include "w5500_driver.h"
#include "usart.h"
#include "string.h"
#include "sys_config.h"

/* --- External Data Pools --- */
extern uint16_t g_modbus_regs[512]; 
extern uint16_t g_waveform_data[CH_NUM][POINTS_PER_PERIOD]; 

/* --- Global Protocol Dispatch Engine --- */
bool Protocol_Dispatch(uint8_t *pIn, uint16_t inLen, DataSource_t source) {
    uint16_t outLen = 0;
    static uint8_t mbap_header[6];
    bool is_success = false; // Flag for successful parse and reply
    
    /* Reference to 5000-byte TX buffer */
    extern uint8_t response_buf[]; 

    if (inLen == 0) return false;

    /* ===================================================================== */
    /* [1] Private Protocol: 0xAA Waveform transparent transmission          */
    /* ===================================================================== */
    /* Only supported over Ethernet */
    if (pIn[0] == 0xAA && source == SOURCE_W5500) {
        response_buf[0] = 0xAA; // Frame header
        uint16_t ptr = 3;       // Reserved for packet length

        /* A. Push diagnostic registers (1~136) */
        for(int i=1; i<=136; i++) {
            response_buf[ptr++] = (g_modbus_regs[i] >> 8) & 0xFF;
            response_buf[ptr++] = g_modbus_regs[i] & 0xFF;
        }

        /* B. Push waveform data */
        for(int ch=0; ch<CH_NUM; ch++) {
            for(int pt=0; pt<POINTS_PER_PERIOD; pt++) {
                response_buf[ptr++] = (g_waveform_data[ch][pt] >> 8) & 0xFF;
                response_buf[ptr++] = g_waveform_data[ch][pt] & 0xFF;
            }
        }

        /* C. Package length and trigger send */
        response_buf[1] = (ptr >> 8) & 0xFF;
        response_buf[2] = ptr & 0xFF;
        
        W5500_Send_Packet(response_buf, ptr); 
        return true;
    }

    /* ===================================================================== */
    /* [2] Standard Modbus Routing                                           */
    /* ===================================================================== */
    if (source == SOURCE_W5500) {
        /* --- Modbus TCP Routing --- */
        if (inLen < 7) return false; 
        
        /* Store MBAP header */
        memcpy(mbap_header, pIn, 6); 
        
        /* Pass to parser engine (No CRC for TCP) */
        outLen = Modbus_Parse_And_Execute(&pIn[6], inLen - 6, &response_buf[6], false);
        
        if (outLen > 0) {
            /* Restore MBAP header and correct PDU length */
            memcpy(response_buf, mbap_header, 6);
            response_buf[4] = (outLen >> 8) & 0xFF;
            response_buf[5] = outLen & 0xFF;
            W5500_Send_Packet(response_buf, outLen + 6); 
            is_success = true;
        }
    } 
    else {
        /* --- Modbus RTU Routing (Robust frame/interference handling) --- */
        uint16_t offset = 0;
        
        while (offset < inLen && offset <= 10 && !is_success) {
            // Quick check: matches device ID or broadcast/forced ID 1
            if (pIn[offset] == (uint8_t)g_modbus_regs[MB_REG_DEV_ID] || pIn[offset] == 0x01) {
                
                // Predict standard frame length to strip tailing garbage
                uint16_t expected_len = 0;
                if ((inLen - offset) >= 2) {
                    uint8_t f_code = pIn[offset + 1];
                    if (f_code == 0x03 || f_code == 0x04 || f_code == 0x06) {
                        expected_len = 8;
                    } else if (f_code == 0x10) {
                        if ((inLen - offset) >= 7) {
                            expected_len = 7 + pIn[offset + 6] + 2; // 9 + data bytes
                        }
                    } else {
                        expected_len = inLen - offset; // Revert if unpredictable
                    }
                }

                // Parse CRC only if length is met
                if (expected_len > 0 && (inLen - offset) >= expected_len) {
                    outLen = Modbus_Parse_And_Execute(&pIn[offset], expected_len, response_buf, true);
                
                    if (outLen > 0) {
                        UART_Send_Data(response_buf, outLen); 
                        is_success = true; // Reply successful, stop searching
                    }
                }
            }
            offset++; // If frame is incorrect, move one byte forward and try again
        }
    }
    return is_success;
}
