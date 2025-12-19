#ifndef HANDLER_HANDLERS_ARP_SOCKET_H
#define HANDLER_HANDLERS_ARP_SOCKET_H

#include <stdint.h>
#include <stdbool.h>

struct arp_socket_t;
struct arp_socket_operations_t {
    bool (*send)(struct handler_t* handler, struct arp_socket_t* socket, uint32_t connection_id, void* buffer, uint64_t size);
    struct arp_status_t* (*status)(struct handler_t* handler, struct arp_socket_t* socket);
};

struct arp_socket_t {
    // next handler must either write data back to the interface, or store the incoming package for later processing, before returning. 
    // There is no guarantee that the data will not be freed after returning from the next_handler->operations.read call.
    struct handler_t* next_handler;

    struct arp_socket_operations_t operations;

    bool passthrough; // if true, then all incoming packets are passed to next handler without processing
};


struct arp_socket_t* arp_create_socket(struct handler_t* next_handler, bool passthrough);

void arp_set_socket(struct arp_socket_t* socket);

#endif // HANDLER_HANDLERS_ARP_SOCKET_H