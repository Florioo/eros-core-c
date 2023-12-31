#include "eros_core.h"
#include "eros_esp_logging.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 512
#define MAX_LOGGING_STREAMS 10
#define LOG_PRINTF_STATEMENTS //If defined, eros will log printf statements otherwise they will be discared
// Static buffer for logging, log messages should be less than 512 bytes,
// otherwise they will be truncated

typedef struct {
    eros_stream_t * eros;
    uint8_t channel;
} eros_logging_config_t;


static eros_logging_config_t eros_logging_config[MAX_LOGGING_STREAMS] = {0};
static  int eros_logging_config_count = 0;
int eros_esp_log_vprintf(const char *fmt, va_list arguments);
int eros_esp_log_vprintf_handler(const char *fmt, va_list arguments);
int stdout_handler(void *_, const char *data, int size);



/**
 * @brief Setup eros logging
 * 
 * @param eros Eros stream to log to
 * @param channel Eros channel to log to
 */
void eros_esp_logging_setup(bool keep_original_logging)
{
    // Reroute logging of the standard ESP to eros
    if (keep_original_logging){
        esp_log_set_vprintf(eros_esp_log_vprintf_handler);
    }else{
        esp_log_set_vprintf(eros_esp_log_vprintf);
    }

    // Reroute logging to stdout to eros (or disable it, depending on LOG_PRINTF_STATEMENTS)
    char * stdout_buf = (char *)malloc(128);
    fclose(stdout);
    stdout = fwopen(NULL, &stdout_handler);
    setvbuf(stdout, stdout_buf, _IOLBF, 128);

}

/**
 * @brief Add a stream to log to
 * 
 * @param eros Eros stream to log to
 * @param channel Eros channel to log to
 */
void eros_esp_logging_add_stream(eros_stream_t * eros, uint8_t channel)
{
    // Save the config
    eros_logging_config[eros_logging_config_count].eros = eros;
    eros_logging_config[eros_logging_config_count].channel = channel;
    eros_logging_config_count++;
}

/**
 * @brief Remove a stream from logging
 * 
 * @param eros Eros stream to remove, if NULL remove all
*/
void eros_esp_logging_remove_stream(eros_stream_t * eros)
{

    if (eros == NULL)
    {
            // Remove all
            for (uint8_t i = 0; i < MAX_LOGGING_STREAMS; i++)
            {
                eros_logging_config[i].eros = NULL;
                eros_logging_config[i].channel = 0;
            }
            eros_logging_config_count = 0;
            return;
    }

    // Search for the eros streams and remove them
    for (uint8_t i = 0; i < MAX_LOGGING_STREAMS; i++)
    {
        if (eros_logging_config[i].eros == eros)
        {
            eros_logging_config[i].eros = NULL;
            eros_logging_config[i].channel = 0;
        }
    }

    // Restucture the array so there are no gaps
    for (uint8_t i = 0; i < MAX_LOGGING_STREAMS; i++)
    {
        if (eros_logging_config[i].eros == NULL)
        {
            // Shift the array down
            for (uint8_t j = i; j < MAX_LOGGING_STREAMS-1; j++)
            {
                eros_logging_config[j].eros = eros_logging_config[j+1].eros;
                eros_logging_config[j].channel = eros_logging_config[j+1].channel;
            }
            // decrement the count
            eros_logging_config_count--;
        }
    }
}



/**
 * @brief This is the eros vprintf function
 * @details This function is called by the ESP logging system when a log is made. 
 * This function then sends the log to eros.
 * 
 * @param fmt The format of the log
 * @param arguments The arguments of the log
 * @return int The length of the log
*/
int eros_esp_log_vprintf(const char *fmt, va_list arguments) 
{
    //Multithreading ISSUE:
    static char buffer[BUFFER_SIZE];
    int length = vsnprintf(buffer, sizeof(buffer), fmt, arguments);

    // Send the log to all eros streams, in this case we can't use eros_transmit_inplace because
    // ite modifies the buffer
    for (uint8_t i = 0; i < eros_logging_config_count; i++)
    {
        eros_transmit(eros_logging_config[i].eros, eros_logging_config[i].channel, (uint8_t *) buffer, length);
    }
    return length;
}

/**
 * @brief This is the stdout handler, it repalces the default stdout handler
 * @details This replaces the printf output.
*/
int stdout_handler(void *_, const char *data, int size)
{

    #ifdef LOG_PRINTF_STATEMENTS
    // Send the log to all eros streams, in this case we can't use eros_transmit_inplace because
    // ite modifies the buffer
    for (uint8_t i = 0; i < eros_logging_config_count; i++)
    {
        eros_transmit(eros_logging_config[i].eros, eros_logging_config[i].channel, (uint8_t *) data, size);
    }

    #endif // LOG_PRINTF_STATEMENTS
    return size;

}
int eros_esp_log_vprintf_handler(const char *fmt, va_list arguments) 
{
    // Print to stdout
    vprintf(fmt, arguments);
    // Invoke the eros vprintf function
    return eros_esp_log_vprintf(fmt, arguments);
}

