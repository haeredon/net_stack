#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_block_buffer.h"

#include <stdbool.h>

#define TCP_DATA_OFFSET_MASK 0xF0


struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);

#endif // HANDLERS_TCP_H