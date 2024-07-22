#include <arpa/inet.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"

#include "log.h"


void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    rte_free(private);
}

void arp_init_handler(struct handler_t* handler) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) rte_zmalloc("pcap handler private data", sizeof(struct arp_priv_t), 0); 
    handler->priv = (void*) arp_priv;
}


uint16_t arp_response(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    NETSTACK_LOG(NETSTACK_WARNING, "arp_response() called");            
}

uint16_t arp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    NETSTACK_LOG(NETSTACK_INFO, "ARP read handler called.\n");   

    uint8_t packet_idx = packet_stack->write_chain_length;
    struct arp_header_t* header = (struct arp_header_t*) packet_stack->packet_pointers[packet_idx];
    
    packet_stack->response[packet_idx] = arp_response;
    
    // is it a request?
    if(!header->target_hardware_addr) {
        // handler_response(packet_stack);        
    } 
    // is it a gratuitous ARP?
    else if(
        // method 1
        (header->target_protocol_addr == header->sender_protocol_addr && !header->target_hardware_addr) ||
        // method 2
        (header->target_protocol_addr == header->sender_protocol_addr && 
        header->target_hardware_addr == header->sender_hardware_addr)) {
                        
    } else {
        NETSTACK_LOG(NETSTACK_INFO, "Received invalid ARP packet.\n");   
    }
}


struct handler_t* arp_create_handler(void* (*mem_allocate)(const char *type, size_t size, unsigned align)) {
    struct handler_t* handler = (struct handler_t*) mem_allocate("arp handler", sizeof(struct handler_t), 0);	

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;
    handler->operations.response = arp_response;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(0x0806), handler);

    return handler;
}


