#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_block_buffer.h"
#include "handlers/tcp/socket.h"

#include <stdbool.h>

#define TCP_DATA_OFFSET_MASK 0xF0

struct tcp_write_args_t {
    uint32_t connection_id;
    struct tcp_socket_t* socket;
    uint8_t flags;
};


struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);


#endif // HANDLERS_TCP_H