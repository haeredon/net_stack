#include <stdio.h>

#include "server.h"
#include "handlers/tcp/socket.h"
#include "handlers/custom/custom.h"


static void on_receive(uint8_t* data, uint64_t size) {
	printf("On Receive()\n");
}

static void on_connect() {
	printf("On Connect()\n");
}

static void on_close() {
	printf("On Close()\n");
}


void server_start(struct handler_t* tcp_handler, uint32_t ipv4, uint16_t port) {
	uint8_t* response = "Echo\n";
	uint32_t response_length = sizeof("Echo\n");

	struct handler_config_t handler_config = {
        .write = 0 
    };

	struct handler_t* custom_handler = custom_create_handler(&handler_config);
    custom_handler->init(custom_handler, 0);
    custom_set_response(custom_handler, response, response_length);

	struct tcp_socket_t* tcp_socket = tcp_create_socket(custom_handler, port, ipv4, on_receive, on_connect, on_close); 
	tcp_add_socket(tcp_handler, tcp_socket);
}