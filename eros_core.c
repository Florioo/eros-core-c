#include "crc16.h"
#include "cobs.h"
#include "eros_core.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define VERISON 0x00

int eros_attach_receive_callback(eros_stream_t * eros, uint8_t channel, channel_callback_t callback, void * context)
{
    if (channel > 15) {
        return 1;
    }
    eros->callbacks[channel] = callback;
    eros->callback_context[channel] = context;
    return 0;
}

int eros_attach_catch_callback(eros_stream_t * eros, channel_catch_callback_t callback)
{
    eros->catch_callback = callback;
    return 0;
}


int eros_encode(uint8_t channel, uint8_t *input_buffer, uint16_t input_data_len,
                                uint8_t *output_buffer , uint16_t *output_buffer_len)
{
    //Add space for CRC and header
    uint8_t * temp_buffer = malloc(input_data_len + 3);

    // Create the header
    uint8_t header = 0x00;
    header |= (VERISON & 0x03) << 6;
    header |= (channel & 0x0F) << 2;

    // Set the in-place sentinel values
    temp_buffer[0] = header;
    
    // Copy the data to the temp buffer
    memcpy(temp_buffer+1, input_buffer, input_data_len);

    // Calculate the Checksum
    uint16_t checksum = crc16(temp_buffer, input_data_len+1);

    // Add the checksum to the transmit buffer
    temp_buffer[input_data_len + 1] = checksum >> 8;
    temp_buffer[input_data_len + 2] = checksum & 0xFF;

    unsigned int final_len = 0;


    cobs_ret_t ret = cobs_encode(temp_buffer, input_data_len + 3, output_buffer, *output_buffer_len, &final_len);
    
    free(temp_buffer);

    if (ret) {
        return 1;
    }

    *output_buffer_len = final_len;
    return 0;
}

int eros_decode_inplace(uint8_t *channel, uint8_t *buffer, uint16_t *buffer_len){

    uint8_t * buffer_start = buffer;
    // Decode the COBS encoding
    cobs_ret_t ret =  cobs_decode_inplace(buffer, *buffer_len);

    if (ret) {
        return 1;
    }
    
    // Remove the sentinel values
    *buffer_len -= 2;
    buffer = buffer+1;

    // Calculate the Checksum
    uint16_t checksum = crc16(buffer, *buffer_len);

    if (checksum){
        return 1;
    }
    
    // Remove the checksum
    *buffer_len -= 2;

    // Extract the header
    uint8_t header = buffer[0];
    *channel = (header & 0x3C) >> 2;

    // Remove the header
    buffer = buffer+1;
    *buffer_len -= 1;

    // Move the buffer back to the start
    memmove(buffer_start, buffer, *buffer_len);
    // ESP_LOGI(TAG, "Channel: %d", *channel);
    // ESP_LOG_BUFFER_HEXDUMP("DECODED", buffer_start, *buffer_len, ESP_LOG_INFO);
    // Null terminate the string
    buffer_start[*buffer_len] = 0;

    return 0;
}


int eros_transmit_printf(eros_stream_t * eros, uint8_t channel, const char * format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (ret < 0) {
        return 1;
    }

    eros_transmit(eros, channel, (uint8_t *) buffer, ret);
    return 0;
}



int eros_transmit(eros_stream_t * eros, uint8_t channel, const uint8_t * data, size_t length)
{
    if (eros == NULL){
        return 1;
    }
    
    // Note: the buffer is allocated with extra bytes to allow for the extra overhead of cobs encoding
    // it requires 1 extra byte when there are more than 255 consecutive non 0x00 bytes
    // In the current implementation we add 7 extra bytes to allow for a 2048 byte packet length
    // There are also 5 extra bytes needed for the starting cobs byte, intial byte header and checksum and trailing byte
    if (length > 2000) {
        return 1;
    }
    
    uint16_t buffer_size = length+5+7;
    uint8_t * buffer = malloc(buffer_size);
        
    // Check if the buffer was allocated
    if (!buffer) {
        printf("Failed to allocate memory for transmit buffer\n");
        return 1;
    }
    
    // Encode the packet
    int ret = eros_encode(channel, (uint8_t *) data, length, buffer, &buffer_size);
    
    if (ret) {
        printf("Failed to encode packet\n");
        free(buffer);
        return 1;
    }

    // Write to the write function
    eros->write_function(eros->transport_context, buffer, buffer_size);

    free(buffer);
    return 0;
}

int eros_process_rx_packet(eros_stream_t * eros, uint8_t * data, size_t length){
    uint8_t channel;
    uint16_t len = length;

    int ret = eros_decode_inplace(&channel, data , &len);

    if (ret) {
        // ESP_LOGE(TAG, "Decoding failed");
        // ESP_LOG_BUFFER_HEXDUMP("RECEIVED", data, length, ESP_LOG_INFO);
        return 1;
    }
    
    // ESP_LOGI(TAG, "Channel: %d", channel);
    // ESP_LOG_BUFFER_HEXDUMP("RECEIVED", data, len, ESP_LOG_INFO);

    if (eros->callbacks[channel]) {
        if (eros->callback_context[channel]) {
            eros->callbacks[channel](eros,data, len, eros->callback_context[channel]);
        }
        else
        {
            eros->callbacks[channel](eros, data, len, NULL);
        }
    }else if (eros->catch_callback){
        eros->catch_callback(eros, channel, data, len);

    }
    return 0;
}


// This needs to be filled in
uint8_t receive_buffer[256];
uint8_t *receive_buffer_pos = receive_buffer;

int eros_receive_data(eros_stream_t * eros, uint8_t * data, size_t length)
{
    // Add the data to the receive buffer
    memcpy(receive_buffer_pos, data, length);
    receive_buffer_pos += length;

    // Split the receive buffer into packets separated by the 0x00
    uint8_t *start = receive_buffer;
    uint8_t *end = receive_buffer_pos;
    uint8_t *next;

    while ((next = memchr(start, 0x00, end - start))) {
        uint16_t len = next - start + 1;
        eros_process_rx_packet(eros, start, len);
        start = next + 1;

    }

    // Move the remaining data to the start of the buffer
    memmove(receive_buffer, start, end - start);
    receive_buffer_pos = receive_buffer + (end - start);

    return 0;
}
