#include <arpa/inet.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"

#include "util/array.h"

#include "log.h"


void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void arp_init_handler(struct handler_t* handler) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) handler->handler_config->mem_allocate("pcap handler private data", sizeof(struct arp_priv_t)); 
    handler->priv = (void*) arp_priv;
}


uint16_t arp_response(struct packet_stack_t* packet_stack, struct response_buffer_t response_buffer, struct interface_t* interface) {    
    NETSTACK_LOG(NETSTACK_WARNING, "arp_response() called");            
}

uint16_t arp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    NETSTACK_LOG(NETSTACK_INFO, "ARP read handler called.\n");   

    uint8_t packet_idx = packet_stack->write_chain_length;
    struct arp_header_t* header = (struct arp_header_t*) packet_stack->packet_pointers[packet_idx];
    
    packet_stack->response[packet_idx] = arp_response;
    
    uint8_t target_hardware_addr_is_zero = array_is_zero(header->target_hardware_addr, ETHERNET_MAC_SIZE);
    
    // is it a request?
    if(target_hardware_addr_is_zero) {
        NETSTACK_LOG(NETSTACK_INFO, "ARP Probe.\n");   
        handler_response(packet_stack, interface);        
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


struct handler_t* arp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("arp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;
    handler->operations.response = arp_response;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(0x0806), handler);

    return handler;
}


