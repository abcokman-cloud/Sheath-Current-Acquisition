#ifndef __PROTOCOL_DISPATCHER_H
#define __PROTOCOL_DISPATCHER_H

#include <stdint.h>
#include <stdbool.h>

/* --- Data Source Enum --- */
typedef enum {
    SOURCE_UART,   /* UART Modbus RTU */
    SOURCE_W5500   /* W5500 Ethernet */
} DataSource_t;

/* --- Protocol Dispatch Engine --- */
bool Protocol_Dispatch(uint8_t *pIn, uint16_t inLen, DataSource_t source);

#endif /* __PROTOCOL_DISPATCHER_H */
