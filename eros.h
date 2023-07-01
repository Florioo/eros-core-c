#include <stdint.h>
#include <stddef.h>
#ifndef EROS_H
#define EROS_H

typedef uint8_t (*channel_callback_t)(void * eros, uint8_t *data, uint16_t length, void * context);
typedef void (*channel_catch_callback_t)(void * eros,uint8_t channel, uint8_t *data, uint16_t length);
typedef void (*io_write_function_t)(void* transport_context, const uint8_t *data, uint16_t length);
typedef int (*io_read_function_t)(void* transport_context, uint8_t *data, uint16_t length);

typedef struct eros_stream_t {
    void * transport_context;
    io_write_function_t write_function;
    io_read_function_t read_function;
    channel_callback_t callbacks[16];
    void * callback_context[16];
    channel_catch_callback_t catch_callback;
} eros_stream_t;


int eros_encode_inplace(uint8_t channel, uint8_t *buffer, uint16_t *buffer_len, uint16_t buffer_size);
int eros_attach_receive_callback(eros_stream_t * eros, uint8_t channel, channel_callback_t callback, void * context);
int eros_decode_inplace(uint8_t *channel, uint8_t *buffer, uint16_t *buffer_len);
int eros_transmit_inplace(eros_stream_t * eros, uint8_t channel, const uint8_t * data, size_t length);
int eros_transmit_with_prefix(eros_stream_t * eros, uint8_t channel, uint8_t prefix, const uint8_t * data, size_t length);
int eros_transmit(eros_stream_t * eros, uint8_t channel, const uint8_t * data, size_t length);
int eros_process_rx_packet(eros_stream_t * eros, uint8_t * data, size_t length);
int eros_attach_catch_callback(eros_stream_t * eros, channel_catch_callback_t callback);
int eros_receive_data(eros_stream_t * eros, uint8_t * data, size_t length);

#endif // EROS_H