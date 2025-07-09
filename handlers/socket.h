#ifndef HANDLER_SOCKET_H
#define HANDLER_SOCKET_H

#include "handlers/handler.h"

#include <stdint.h>
#include <stdbool.h>

struct socket_operations_t {
    uint32_t (*open)(struct tcp_socket_t* socket);
    bool (*send)(struct tcp_socket_t* socket, uint32_t connection_id);
    void (*receive)(uint8_t* data, uint64_t size); // provided by user of socket. This is a callback function 
    void (*close)(struct tcp_socket_t* socket, uint32_t connection_id);
    void (*abort)(struct tcp_socket_t* socket, uint32_t connection_id);
    void (*status)(struct tcp_socket_t* socket, uint32_t connection_id);
};

struct socket_t {
    struct socket_operations_t operations;

    struct interface_t* interface;

    struct handlers_t* handlers[10];
    void* handler_args[10];
    uint8_t depth;
};


#endif // HANDLER_SOCKET_H