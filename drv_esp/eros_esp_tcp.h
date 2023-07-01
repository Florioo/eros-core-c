#include "eros.h"

#ifndef EROS_ESP_TCP_H
#define EROS_ESP_TCP_H

void eros_tcp_read_task(void* context);
int eros_tcp_read(void* context, uint8_t *data, uint16_t length);
void eros_tcp_write(void* context, uint8_t *data, uint16_t length);
eros_stream_t * eros_init_tcp(int port);


#endif // EROS_ESP_UART_H