#include <stdio.h>  // Include the standard input/output library for I/O operations.
#include <string.h>  // Include the string library for string operations.
#include <stdlib.h>  // Include the standard library for memory allocation and other functions.
#include "hardware/gpio.h"  // Include the Pico hardware GPIO library.

#include "pico/cyw43_arch.h"  // Include a custom Pico library for a specific hardware component.
#include "pico/stdlib.h"  // Include the Pico standard library for Pico-specific functions.

#include "lwip/ip4_addr.h"  // Include the lwIP library for IPv4 address handling.
#include "lwip/pbuf.h"  // Include the lwIP library for packet buffer management.
#include "lwip/tcp.h"  // Include the lwIP library for TCP communication.

#include "FreeRTOS.h"  // Include the FreeRTOS library for real-time operating system functionality.
#include "task.h"  // Include the FreeRTOS library for task management.

#define TCP_PORT 4242  // Define a constant for the TCP port number the server will use.
#define BUF_SIZE 2048  // Define a constant for the size of the data buffer.

typedef struct TCP_SERVER_T_ {  // Define a custom data structure for the TCP server.
    struct tcp_pcb *server_pcb;  // Pointer to the server's TCP protocol control block.
    struct tcp_pcb *client_pcb;  // Pointer to the client's TCP protocol control block.
    bool complete;  // Flag to indicate completion.
    char buffer_sent[BUF_SIZE];  // Buffer for sent data.
    uint8_t buffer_recv[BUF_SIZE];  // Buffer for received data.
    int sent_len;  // Length of sent data.
    int recv_len;  // Length of received data.
    int run_count;  // Counter for server run iterations.
} TCP_SERVER_T;

static TCP_SERVER_T* tcp_server_init(void) {  // Initialize the TCP server state.
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));  // Allocate memory for the server state.
    if (!state) {  // Check if memory allocation failed.
        return NULL;
    }
    return state;
}

static void tcp_server_err(void *arg, err_t err) {  // Handle TCP server errors.
    if (err != ERR_ABRT) {  // Check if the error is not an abort error.
        printf("Error code: %d\n", err);  // Print the error code.
    }
}

err_t tcp_server_send_data(struct pbuf *p, void *arg, struct tcp_pcb *tpcb) {  // Send data over the TCP connection.
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;  // Retrieve the server state from the argument.
    int len = p->tot_len;
    if (p->tot_len > BUF_SIZE) len = BUF_SIZE;
    strncpy(state->buffer_sent, p->payload, len);  // Copy "ok" to the send buffer.
    cyw43_arch_lwip_check();  // Check the lwIP stack for readiness.
    err_t err = tcp_write(tpcb, state->buffer_sent, len, TCP_WRITE_FLAG_COPY);  // Write data to the TCP connection.
    if (err != ERR_OK) {  // Check if the write operation failed.
        printf("Failed to write data!\n");  // Print an error message.
        return err;
    }
    else {
        printf("Data sent successfully!\n");  // Print a success message.
        return ERR_OK;
    }
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {  // Receive data from the TCP connection.
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;  // Retrieve the server state from the argument.
    if (!p) {  // Check if the received packet buffer is invalid.
        printf("Something went wrong!\n");  // Print an error message.
        return ERR_BUF;
    }
    cyw43_arch_lwip_check();  // Check the lwIP stack for readiness.
    if (strlen(p->payload) == 0) {  // Check if the received data is empty.
        pbuf_free(p);  // Free the packet buffer.
        return ERR_OK;
    }
    else if (p->tot_len > 0) {  // Check if the total length of received data is greater than 0.
        printf("Buffer value: %s\n", p->payload);  // Print the received data.
    }
    tcp_server_send_data(p, arg, state->client_pcb);  // Send a response to the client.
    pbuf_free(p);  // Free the packet buffer.
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {  // Handle incoming client connections.
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;  // Retrieve the server state from the argument.
    if (err != ERR_OK || client_pcb == NULL) {  // Check for errors or invalid client protocol control block.
        printf("Failure in accept\n");  // Print an error message.
        return ERR_VAL;
    }
    printf("Client connected\n");  // Print a message indicating a successful client connection.
    state->client_pcb = client_pcb;  // Store the client's protocol control block.
    tcp_arg(client_pcb, state);  // Set the argument for the client's TCP connection.
    tcp_recv(client_pcb, tcp_server_recv);  // Set the callback for receiving data on the client connection.
    tcp_err(client_pcb, tcp_server_err);  // Set the callback for handling errors on the client connection.
    return ERR_OK;
}

static bool tcp_server_open(void *arg) {  // Open and start the TCP server.
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;  // Retrieve the server state from the argument.
    printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);  // Print a message with server details.
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);  // Create a new TCP protocol control block.
    if (!pcb) {  // Check if the PCB creation failed.
        printf("Failed to create pcb\n");  // Print an error message.
        return false;
    }
    err_t err = tcp_bind(pcb, NULL, TCP_PORT);  // Bind the PCB to the specified port.
    if (err) {  // Check if the binding operation failed.
        printf("Failed to bind to port %u\n", TCP_PORT);  // Print an error message.
        return false;
    }
    state->server_pcb = tcp_listen_with_backlog(pcb, 1);  // Listen for incoming connections with a backlog of 1.
    if (!state->server_pcb) {  // Check if listening failed.
        printf("Failed to listen\n");  // Print an error message.
        if (pcb) {
            tcp_close(pcb);  // Close the PCB if it was created.
        }
        return false;
    }
    tcp_arg(state->server_pcb, state);  // Set the argument for the server PCB.
    tcp_accept(state->server_pcb, tcp_server_accept);  // Set the callback for accepting client connections.
    return true;
}

void start_server(__unused void *params) {  // Entry point for the server task.
    if (cyw43_arch_init()) {  // Initialize a specific hardware component.
        printf("Failed to initialise\n");  // Print an error message.
        return;
    }
    cyw43_arch_enable_sta_mode();  // Enable a specific Wi-Fi mode.
    printf("Connecting to Wi-Fi...\n");  // Print a message indicating a Wi-Fi connection attempt.
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {  // Attempt to connect to Wi-Fi.
        printf("Failed to connect.\n");  // Print an error message on Wi-Fi connection failure.
        return;
    }
    else {
        printf("Connected.\n");  // Print a message indicating a successful Wi-Fi connection.
    }
    TCP_SERVER_T *state = tcp_server_init();  // Initialize the TCP server state.
    if (!state) {  // Check if state initialization failed.
        printf("Failed to allocate state\n");  // Print an error message.
        return;
    }
    if (!tcp_server_open(state)) {  // Start the TCP server.
        printf("Failed to start server\n");  // Print an error message on server start failure.
        return;
    }
    while (true) {  // Enter an infinite loop.
        vTaskDelay(1000);  // Delay the task for 1000 milliseconds.
    }
}

int main() {  // Main function of the program.
    stdio_init_all();  // Initialize standard I/O.
    sleep_ms(5000);  // Sleep for 5000 milliseconds.
    TaskHandle_t server_task;  // Create a task handle for the server task.
    xTaskCreate(start_server, "StartServer", configMINIMAL_STACK_SIZE, NULL, 4, &server_task);  // Create the server task.
    vTaskStartScheduler();  // Start the FreeRTOS task scheduler.
    return 0;  // Return 0 to indicate successful program execution.
}
