#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_block_buffer.h"

#include <stdbool.h>

#define TCP_DATA_OFFSET_MASK 0xF0


struct tcp_socket_t {
    uint16_t listening_port;
    // struct interface_t* interface;

    // uint8_t* receive_buffer;
    // void (*notify_receive)(struct request_t* request);

    struct transmission_control_block_t trans_control_block;
};

bool tcp_add_socket(struct tcp_socket_t* socket, struct handler_t* handler);

struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);

#endif // HANDLERS_TCP_H