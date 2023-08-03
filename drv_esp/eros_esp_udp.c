#include "eros_core.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "string.h"

#define MAX_UDP_PAYLOAD 521

typedef struct {
    int sock;
    struct sockaddr_storage peer_addr;
} udp_context_t;

static const char *TAG = "eros_esp_udp";

void udp_read_task(void* context);
int udp_read(void* context, uint8_t *data, uint16_t length);
void udp_write(void* context, uint8_t *data, uint16_t length);


eros_stream_t * eros_init_udp(int port)
{
    // Allocate the transport context
    udp_context_t * udp_context = malloc(sizeof(udp_context_t));
    memset(udp_context, 0, sizeof(udp_context_t));

    // Initialize the eros stream
    eros_stream_t * eros = malloc(sizeof(eros_stream_t));
    memset(eros, 0, sizeof(eros_stream_t));

    eros->transport_context = udp_context;
    eros->write_function = (void*)udp_write;
    eros->read_function = (void*)udp_read;

    // Create a UDP socket
    udp_context->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_context->sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return NULL;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //Accept any incoming address
    dest_addr.sin_family = AF_INET;                 //IPv4
    dest_addr.sin_port = htons(port);               //Port to listen on

    // Bind the socket to the port
    int err = bind(udp_context->sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        return NULL;
    }

    // Start the read task
    xTaskCreate(udp_read_task, "eros_udp_read_task", 1024*6, eros, 10, NULL);
    
    return eros;
}

void udp_write(void* context, uint8_t *data, uint16_t length)
{
    // Check if peer_addr is set
    if (((udp_context_t*) context)->peer_addr.ss_family == 0)
    {
        // Logging the error causes a stack overflow, if the logging is attached to the udp stream
        // ESP_LOGE(TAG, "Peer address not set");
        return;
    }

    udp_context_t * udp_context = (udp_context_t*) context;
    // Send data to the UDP socket
    sendto(udp_context->sock, data, length, 0, (struct sockaddr *)&udp_context->peer_addr, sizeof(udp_context->peer_addr));
}

int udp_read(void* context, uint8_t *data, uint16_t length)
{
    udp_context_t * udp_context = (udp_context_t*) context;
    socklen_t socklen = sizeof(udp_context->peer_addr);
    // Read data from the UDP socket
    int ret =  recvfrom(udp_context->sock, data, length - 1, 0, (struct sockaddr *)&udp_context->peer_addr, &socklen);
    printf("Received %d bytes\n", ret);
    return ret;
}

void udp_read_task(void* context)
{
    eros_stream_t *eros_stream  = (eros_stream_t *) context;
    uint8_t buffer[MAX_UDP_PAYLOAD];
    while (true)
    {
        int len = udp_read(eros_stream->transport_context, buffer, sizeof(buffer));
        if (len == 0)
        {
            ESP_LOGE(TAG, "No data received");
            continue;
        }
        eros_receive_data(eros_stream ,buffer, len);
    }
}