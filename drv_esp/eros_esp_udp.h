#include "eros_core.h"
#include "driver/uart.h"

#ifndef EROS_ESP_UDP_H
#define EROS_ESP_UDP_H

void udp_read_task(void* context);
int udp_read(void* context, uint8_t *data, uint16_t length);
void udp_write(void* context, uint8_t *data, uint16_t length);
eros_stream_t * eros_init_udp(int port);


#endif // EROS_ESP_UART_H