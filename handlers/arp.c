#include <arpa/inet.h>
#include <string.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"

#include "util/array.h"

#include "log.h"

/*
 * Address Resolution Protocol handler. Only supports IPv4 over ethernet.
 * Specification: RFC 826
 */


// very basic slow implementation of ARP mapping. Should be optimized when more is known about use cases
struct arp_entry_t {
    uint32_t ipv4;
    uint8_t mac[ETHERNET_MAC_SIZE];
};

struct arp_entry_t* arp_resolution_list[256]; 


struct arp_entry_t* arp_get_mapping(uint8_t mac[ETHERNET_MAC_SIZE]) {
    for(uint16_t i = 0 ; i < 256 ; i++) {
        struct arp_entry_t* entry = arp_resolution_list[i];

        if(entry) {
            if(memcmp(entry->mac, mac, ETHERNET_MAC_SIZE) == ETHERNET_MAC_SIZE) {
                return entry;
            }
        } else {
            arp_resolution_list[i] = 
            return arp_resolution_list[i];
        }        
    }
    arp_resolution_list[0] = 
}

void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void arp_init_handler(struct handler_t* handler) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) handler->handler_config->mem_allocate("pcap handler private data", sizeof(struct arp_priv_t)); 
    handler->priv = (void*) arp_priv;
}


uint16_t arp_handle_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, struct interface_t* interface) {    
    struct arp_header_t* request_header = (struct arp_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct arp_header_t* response_header = (struct arp_header_t*) response_buffer->buffer;

    response_header->hdw_type = request_header->hdw_type;
    response_header->pro_type = request_header->pro_type;
    response_header->hdw_addr_length = request_header->hdw_addr_length;
    response_header->pro_addr_length = request_header->pro_addr_length;
    response_header->operation = request_header->operation = ARP_OPERATION_RESPOENSE;
    memcpy(response_header->sender_hardware_addr, request_header->target_hardware_addr, ETHERNET_MAC_SIZE);
    response_header->sender_protocol_addr = request_header->target_protocol_addr;
    memcpy(response_header->target_hardware_addr, request_header->sender_hardware_addr, ETHERNET_MAC_SIZE);
    response_header->target_protocol_addr = request_header->sender_protocol_addr;

    uint16_t num_bytes_written = sizeof(struct arp_header_t);
    response_buffer->offset += num_bytes_written;
    
    return num_bytes_written;
}

uint16_t arp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    NETSTACK_LOG(NETSTACK_INFO, "ARP read handler called.\n");   

    uint8_t packet_idx = packet_stack->write_chain_length;
    struct arp_header_t* header = (struct arp_header_t*) packet_stack->packet_pointers[packet_idx];

    if(header->hdw_type == ARP_HDW_TYPE_ETHERNET) {
        if(header->pro_type == 0x0800) {
            struct arp_entry_t* mapping = arp_get_mapping(header->sender_protocol_addr);

            memcpy(mapping->mac, header->sender_hardware_addr, ETHERNET_MAC_SIZE);                
            mapping->ipv4 = header->sender_protocol_addr;
            
            if(header->operation == ARP_OPERATION_REQUEST && header->target_protocol_addr == interface->ipv4_addr) {
                packet_stack->response[packet_idx] = arp_handle_response;
                handler_response(packet_stack, interface);                                                                                
            }
        }
    }
}


struct handler_t* arp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("arp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(0x0806), handler);

    return handler;
}


