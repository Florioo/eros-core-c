#include "eros.h"
#include "driver/uart.h"

#ifndef EROS_ESP_UART_H
#define EROS_ESP_UART_H
eros_stream_t * eros_init_uart( uart_port_t uart_num);


#endif // EROS_ESP_UART_H