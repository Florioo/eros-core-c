#include "eros.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "string.h"

#define MAX_TCP_PAYLOAD 512 //Max size of a single TCP packet

typedef struct {
    int listen_socket;
    int client_socket;
} tcp_context_t;

static const char *TAG = "eros_esp_tcp";

void eros_tcp_read_task(void* context);
int eros_tcp_read(void* context, uint8_t *data, uint16_t length);
void eros_tcp_write(void* context, uint8_t *data, uint16_t length);


eros_stream_t * eros_init_tcp(int port)
{
    // Allocate the transport context
    tcp_context_t * tcp_context = malloc(sizeof(tcp_context_t));
    memset(tcp_context, 0, sizeof(tcp_context_t));

    // Initialize the eros stream
    eros_stream_t * eros = malloc(sizeof(eros_stream_t));
    memset(eros, 0, sizeof(eros_stream_t));

    eros->transport_context = tcp_context;
    eros->write_function = eros_tcp_write;
    eros->read_function = eros_tcp_read;
    
    // Create a TCP socket
    tcp_context->listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (tcp_context->listen_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return NULL;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //Accept any incoming address
    dest_addr.sin_family = AF_INET;                 //IPv4
    dest_addr.sin_port = htons(port);               //Port to listen on

    // Bind the socket to the port
    int err = bind(tcp_context->listen_socket, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        return NULL;
    }
    
    // Listen for incoming connections
    if (listen(tcp_context->listen_socket, 1) != 0) {
        ESP_LOGE(TAG, "Socket unable to listen: errno %d", errno);
        return NULL;
    }

    // Start the read task
    xTaskCreate(eros_tcp_read_task, "eros_tcp_read_task", 1024*6, eros, 10, NULL);
    
    return eros;
}

void eros_tcp_write(void* context, uint8_t *data, uint16_t length)
{
    // Do not use printf statements in this function because it will cause a stack overflow
    // if we are logging trough TCP
    tcp_context_t * tcp_context = (tcp_context_t*) context;
    
    // Check if socket exists
    if (!tcp_context->client_socket)
    {     
        return;
    }
    
    int err = send(tcp_context->client_socket, data, length, MSG_NOSIGNAL);
    
    if (err < 0) {
        // tcp_context->client_socket = 0;
    }
}

int eros_tcp_read(void* context, uint8_t *data, uint16_t length)
{
    tcp_context_t * tcp_context = (tcp_context_t*) context;
    // Read data from the TCP socket
    return recv(tcp_context->client_socket, data, length - 1, 0);
}

/**
 * @brief Task that manages the a  single TCP connection and waits for incoming data
 * It can only handle one connection at a time
 * @param context: The eros_stream_t context
*/
void eros_tcp_read_task(void* context)
{
    eros_stream_t *eros_stream  = (eros_stream_t *)context;
    tcp_context_t * tcp_context = (tcp_context_t *)eros_stream->transport_context;
    uint8_t buffer[MAX_TCP_PAYLOAD];
    
    while (true)
    {
        ESP_LOGI(TAG, "Waiting for Conenction");
        
        // Accept a new connection
        tcp_context->client_socket = accept(tcp_context->listen_socket,NULL,NULL);
        
        // Check if the socket is opened properly
        if (tcp_context->client_socket < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: '%s'", strerror(errno));
            continue;
        }

        // Set TCP_NODELAY
        int flag = 1;
        if (setsockopt(tcp_context->client_socket, IPPROTO_TCP , TCP_NODELAY, &flag, sizeof(flag)) < 0) {
            ESP_LOGE(TAG, "Unable to set TCP_NODELAY: '%s'", strerror(errno));
            continue;
        }

        ESP_LOGI(TAG, "Socket Accepted");

        // Read data from the TCP socket
        while (true)
        {
            
            int len = eros_tcp_read(eros_stream->transport_context, buffer, sizeof(buffer));
            
            if (len == 0)
            {
                // ESP_LOGE(TAG, "No data received, socket closed");
                break;
            }
            
            if (len < 0)
            {
                if (errno == ECONNRESET) {
                    // Connection was closed by the client
                    break;
                }

                else{
                    ESP_LOGE(TAG, "Error occurred during receiving: errno [%d]'%s'", errno, strerror(errno));
                    break;
                }
            }

            eros_receive_data(eros_stream ,buffer, len);
        }

        // Close the socket
        if (tcp_context->client_socket != 0) {

            //Read and write operations are terminated
            shutdown(tcp_context->client_socket, SHUT_RDWR);

            //Close the socket
            close(tcp_context->client_socket);
            
            tcp_context->client_socket = 0;

            // ESP_LOGI(TAG, "Socket Closed");
        }

    }
}
