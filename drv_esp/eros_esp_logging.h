#include "eros_core.h"
#include <stdbool.h>

#ifndef EROS_ESP_LOGGING_H
#define EROS_ESP_LOGGING_H

void eros_esp_logging_setup(bool keep_original_logging);
void eros_esp_logging_add_stream(eros_stream_t * eros, uint8_t channel);
void eros_esp_logging_remove_stream(eros_stream_t * eros);
#endif // EROS_LOGGING_H