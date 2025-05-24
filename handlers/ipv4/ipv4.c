#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "handlers/ipv4/ipv4.h"
#include "handlers/ethernet/ethernet.h"
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

void ipv4_init_handler(struct handler_t* handler, void* priv_config) {
    struct ipv4_priv_t* ipv4_priv = (struct ipv4_priv_t*) handler->handler_config->mem_allocate("ipv4 handler private data", sizeof(struct ipv4_priv_t)); 
    handler->priv = (void*) ipv4_priv;
}

uint16_t ipv4_calculate_checksum(const struct ipv4_header_t* header) {    
    uint16_t sum = 0;
    const uint8_t length = (header->flags_1 & IPV4_IHL_MASK) * sizeof(uint16_t);
    uint16_t* data = (uint16_t*) header;

    for(uint8_t i = 0 ; i < length ; ++i) {
        uint16_t byte = data[i];
        
        sum += byte;
        if(sum < byte) {
            sum++;
        }
    }

    return ((uint16_t) ~sum);
}

bool write(struct packet_stack_t* packet_stack, struct package_buffer_t* buffer, uint8_t stack_idx, struct interface_t* interface, const struct handler_t* handler) {
    struct ipv4_header_t* packet_stack_header = (struct ipv4_header_t*) packet_stack->packet_pointers[stack_idx];
    struct ipv4_header_t* response_header = (struct ipv4_header_t*) ((uint8_t*) buffer->buffer + buffer->data_offset - sizeof(struct ethernet_header_t));

    if(buffer->data_offset < sizeof(struct ipv4_header_t)) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for ipv4 header in buffer\n");         
        return false;   
    };

    response_header->flags_1 = 0x45; // version 4 and length 20
    response_header->flags_2 = 0; // type of service is not supported
    response_header->total_length = 0;
    response_header->identification = 0; // don't use
    response_header->flags_3 = 64; // don't fragment, it's not supported
    response_header->time_to_live = 64; // have this as configuration
    response_header->protocol = packet_stack_header->protocol;
    response_header->header_checksum = 0; // leave 0 so that it's ready for checksum calculation further down
    response_header->source_ip = packet_stack_header->destination_ip;
    response_header->destination_ip = packet_stack_header->source_ip;
    response_header->total_length = htons(buffer->size - buffer->data_offset);
    response_header->header_checksum = ipv4_calculate_checksum(response_header);

    buffer->data_offset -= sizeof(struct ipv4_header_t);
   
    // if this is the bottom of the packet stack, then write to the interface
    if(!stack_idx) {
        handler->handler_config->write(buffer, interface, 0);
    } else {
        const struct handler_t* next_handler = packet_stack->handlers[--stack_idx];
        next_handler->operations.write(packet_stack, buffer, stack_idx, interface, next_handler);        
    }
    
    return true;
}

uint16_t read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct ipv4_header_t* header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_idx];

    if(ipv4_calculate_checksum(header)) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 checksum wrong: Dropping package.\n");          
        return 1;
    }

    uint8_t IHL = header->flags_1 & IPV4_IHL_MASK;
    if(IHL != 5) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 with options not supported: Dropping package.\n");   
        return 2;
    }

    if(!header->time_to_live) {
        NETSTACK_LOG(NETSTACK_INFO, "IPv4 Time to Live exceded: Dropping package.\n");   
        return 3;
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
            return 4;
        }
    }

    return 0;
}


struct handler_t* ipv4_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("ipv4 handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = ipv4_init_handler;
    handler->close = ipv4_close_handler;

    handler->operations.read = read;
    handler->operations.write = write;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(ETHERNET_TYPE_IPV4), handler);

    return handler;
}


