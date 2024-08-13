#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "handlers/ipv4.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"
#include "handlers/protocol_map.h"


#include "log.h"

/*
 * Internet Protocol Version 4 handler. No support for options field or fragmentation.
 * Specification: RFC 791
 * 
 * Ignoring:    
 *      Type of Service
 * 
 * Will fail on:
 *      Options 
 *      Fragmentation
 * 
 * 
 */


struct key_val_t ipv4_key_vals[IP_NUM_PROTOCOL_TYPE_ENTRIES];
struct priority_map_t ip_type_to_handler = { 
    .max_size = IP_NUM_PROTOCOL_TYPE_ENTRIES,
    .map = ipv4_key_vals
};

void ipv4_close_handler(struct handler_t* handler) {
    struct ipv4_priv_t* private = (struct ipv4_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void ipv4_init_handler(struct handler_t* handler) {
    struct ipv4_priv_t* ipv4_priv = (struct ipv4_priv_t*) handler->handler_config->mem_allocate("pcap handler private data", sizeof(struct ipv4_priv_t)); 
    handler->priv = (void*) ipv4_priv;
}


uint16_t ipv4_handle_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, struct interface_t* interface) { 
    struct ipv4_header_t* request_header = (struct ipv4_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct ipv4_header_t* response_header = (struct ipv4_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    response_header->flags_1 = 0x4500; // version 4 and length 20
    response_header->flags_2 = 0; // type of service is not supported
    response_header->total_length = ;
    response_header->identification = 0; // don't use
    response_header->flags_3 = 0x2; // don't fragment, it's not supported
    response_header->time_to_live = 64; // have this as configuration
    response_header->protocol = request_header->protocol;
    response_header->header_checksum = 0; // leave 0 so that it's ready for checksum calculation further down
    response_header->source_ip = request_header->destination_ip;
    response_header->destination_ip = request_header->source_ip;

    uint16_t num_bytes_written = sizeof(struct ipv4_header_t);
    response_buffer->offset += num_bytes_written;
    
    return num_bytes_written;

    return 0;
}

bool ipv4_checksum_verify(const struct ipv4_header_t* header) {    
    uint16_t sum = 0;
    const uint8_t length = (header->flags_1 & 0x0F) * sizeof(uint16_t);
    uint16_t* data = (uint16_t*) header;

    for(uint8_t i = 0 ; i < length ; ++i) {
        uint16_t byte = data[i];
        sum += byte;
        if(sum < byte) {
            sum++;
        }
    }

    return ((uint16_t) ~sum) == 0;
}

uint16_t ipv4_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    NETSTACK_LOG(NETSTACK_INFO, "IPv4 read handler called.\n");   

    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct ipv4_header_t* header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_idx];

    packet_stack->response[packet_idx] = ipv4_handle_response;
 
    if(!ipv4_checksum_verify(header)) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 checksum wrong: Dropping package.\n");          
        return 1;
    }

    uint8_t IHL = header->flags_1 & 0x0F;
    if(IHL != 5) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 with options not supported: Dropping package.\n");   
        return 1;
    }

    if(!header->time_to_live) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 Time to Live exceded: Dropping package.\n");   
        return 1;
    }

    if(header->destination_ip == interface->ipv4_addr) {
        struct handler_t* next_handler = GET_FROM_PRIORITY(
            &ip_type_to_handler,
            header->protocol, 
            struct handler_t
        );

        if(next_handler) {
            // set next buffer pointer for next protocol level     
            packet_stack->packet_pointers[packet_stack->write_chain_length] = ((uint8_t*) header) + sizeof(struct ipv4_header_t);
            
            next_handler->operations.read(packet_stack, interface, next_handler);  
        } else {
            NETSTACK_LOG(NETSTACK_WARNING, "IPv4 received non-supported protocol type: %hx\n", header->protocol);            
        }
    } else {        
        handler_response(packet_stack, interface);            
    }
}


struct handler_t* ipv4_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("ipv4 handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = ipv4_init_handler;
    handler->close = ipv4_close_handler;

    handler->operations.read = ipv4_read;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(ETHERNET_TYPE_IPV4), handler);

    return handler;
}


