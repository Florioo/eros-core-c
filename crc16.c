#include <stdint.h>
#include <stddef.h>

uint16_t crc16(uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    uint16_t polynomial = 0x8005;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}
