#include "handlers/arp/socket.h"
#include "handlers/arp/arp.h"
#include "handlers/handler.h"
#include "util/memory.h"
#include "util/log.h"

#include <pthread.h>
#include <string.h>

static bool send(struct arp_socket_t* socket, uint32_t connection_id, void* buffer, uint64_t size) {
    // not implemented
    return false;
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

struct arp_socket_t* arp_create_socket(struct handler_t* next_handler, bool passthrough) {
    struct arp_socket_t* socket = (struct arp_socket_t*) NET_STACK_MALLOC("arp socket", sizeof(struct arp_socket_t));
    socket->next_handler = next_handler;
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