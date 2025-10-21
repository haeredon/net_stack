#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

#include "handlers/handler.h"

void server_start(struct handler_t* tcp_handler, uint32_t ipv4, uint16_t port);


#endif // SERVER_H