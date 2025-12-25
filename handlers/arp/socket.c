#include "handlers/arp/socket.h"
#include "handlers/arp/arp.h"
#include "handlers/handler.h"
#include "util/memory.h"
#include "util/log.h"

#include <pthread.h>
#include <string.h>

static bool send(struct handler_t* handler, struct arp_socket_t* socket, struct socket_client_args* protocol_stack) {  
    struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) NET_STACK_MALLOC("send package: arp_package", DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct out_packet_stack_t));     
    
    memcpy(out_package_stack->handlers, protocol_stack->handlers, 10 * sizeof(struct handler_t*));
    memcpy(out_package_stack->args, protocol_stack->handler_args, 10 * sizeof(void*));

    out_package_stack->out_buffer.buffer = (uint8_t*) out_package_stack + sizeof(struct out_packet_stack_t);
    out_package_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_package_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;          

    out_package_stack->stack_idx = protocol_stack->depth - 1;

    return handler->operations.write(out_package_stack, socket->interface, handler);
}

static struct arp_status_t* status(struct handler_t* handler, struct arp_socket_t* socket) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    struct arp_resoltion_list_t* arp_resolution_list = &private->resolution_list;
    struct arp_status_t* arp_status = (struct arp_status_t*) NET_STACK_MALLOC("arp status", sizeof(struct arp_status_t));

    pthread_rwlock_rdlock(&arp_resolution_list->lock);
    for (uint32_t i = 0; i < ARP_RESOLUTION_LIST_SIZE; i++) {
        struct arp_entry_t* entry = arp_resolution_list->list[i];

        if(!entry) {
            arp_status->num_arp_entries = i;
            break;
        }

        memcpy(&arp_status->entries[i], entry, sizeof(struct arp_entry_t));
    }

    pthread_rwlock_unlock(&arp_resolution_list->lock);
    return arp_status;     
}

struct arp_socket_t* arp_create_socket(struct handler_t* next_handler, struct interface_t* interface, bool passthrough) {
    struct arp_socket_t* socket = (struct arp_socket_t*) NET_STACK_MALLOC("arp socket", sizeof(struct arp_socket_t));
    socket->next_handler = next_handler;
    socket->interface = interface;
    socket->passthrough = passthrough;
    socket->operations.send = send;
    socket->operations.status = status;
    return socket;
}

void arp_set_socket(struct handler_t* handler, struct arp_socket_t* socket) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    

    if(private->socket) {
        NETSTACK_LOG(NETSTACK_WARNING, "Overwriting existing ARP socket in handler. This is not a thread safe operation.");
    }

    private->socket = socket;
}