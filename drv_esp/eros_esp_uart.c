#include "eros_core.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG __attribute__((unused)) = "eros_esp_uart";

void read_task(void* context);
int uart_read(void* context, uint8_t *data, uint16_t length);
void uart_write(void* context, uint8_t *data, uint16_t length);


eros_stream_t *  eros_init_uart( uart_port_t uart_num, int baudrate)
{

    eros_stream_t * eros = malloc(sizeof(eros_stream_t));
    memset(eros, 0, sizeof(eros_stream_t));

    eros->transport_context = (void*)uart_num;
    eros-> write_function =  (void*)uart_write;
    eros->read_function =  (void*)uart_read;

    
    
    // initializes the UART
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,

    };
    QueueHandle_t uart_queue;
    // Create a FIFO queue for UART events

    ESP_ERROR_CHECK(uart_driver_install(uart_num, 256, 256, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Start the read task
    xTaskCreatePinnedToCore(read_task, "eros_uart_read_task", 1024*4, eros, 2, NULL, 1);
    
    return eros;

}

void uart_write(void* context, uint8_t *data, uint16_t length)
{
    uart_port_t uart_num = (uart_port_t) context;
    uart_write_bytes(uart_num, (const char *) data, length);
}

int uart_read(void* context, uint8_t *data, uint16_t length)
{
    uart_port_t uart_num = (uart_port_t) context;
    return uart_read_bytes(uart_num, data, length, 1);
}


void read_task(void* context)
{
    
    eros_stream_t *eros_stream  = (eros_stream_t *) context;

    uint8_t buffer[256];
    while (true)
    {

        uint16_t len = uart_read(eros_stream->transport_context, buffer, sizeof(buffer));
        if (len > 0)
        {
            // ESP_LOGI(TAG, "Read %d bytes", len);
            eros_receive_data(eros_stream ,buffer, len);
        }
    }
}
