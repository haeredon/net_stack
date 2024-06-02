#include <arpa/inet.h>

#include <rte_malloc.h>
#include <rte_mbuf.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"



void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    rte_free(private);
}

void arp_init_handler(struct handler_t* handler) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) rte_zmalloc("pcap handler private data", sizeof(struct arp_priv_t), 0); 
    handler->priv = (void*) arp_priv;
}

uint16_t arp_read(void* buffer, uint16_t offset, struct interface_t* interface, void* priv) {
    RTE_LOG(INFO, USER1, "ARP read handler called.\n");   

    struct arp_header_t* header = (struct arp_header_t*) buffer;
}

struct handler_t* arp_create_handler(void* (*mem_allocate)(const char *type, size_t size, unsigned align)) {
    struct handler_t* handler = (struct handler_t*) mem_allocate("arp handler", sizeof(struct handler_t), 0);	

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(0x0806), handler);

    return handler;
}
