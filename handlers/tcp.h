#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handler.h"


struct tcp_priv_t {
    int dummy;
};

struct tcp_header_t {
    
} __attribute__((packed, aligned(2)));

struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);





#endif // HANDLERS_TCP_H