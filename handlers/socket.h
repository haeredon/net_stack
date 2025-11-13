#ifndef HANDLER_SOCKET_H
#define HANDLER_SOCKET_H

#include "handlers/handler.h"

#include <stdint.h>
#include <stdbool.h>

struct socket_client_args {
    struct handler_t* handlers[10];
    void* handler_args[10];
    uint8_t depth;
};


#endif // HANDLER_SOCKET_H