#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "handlers/ethernet/ethernet.h"
#include "handlers/protocol_map.h"
#include "handlers/handler.h"
#include "util/memory.h"

 

#include "util/log.h"


struct key_val_t key_vals[ETHERNET_NUM_ETH_TYPE_ENTRIES];
struct priority_map_t ethernet_type_to_handler = { 
    .max_size = ETHERNET_NUM_ETH_TYPE_ENTRIES,
    .map = key_vals
};


void ethernet_init_handler(struct handler_t* handler, void* priv_config) {
    struct ethernet_priv_t* ethernet_priv = (struct ethernet_priv_t*) NET_STACK_MALLOC("pcap handler private data", sizeof(struct ethernet_priv_t)); 
    handler->priv = (void*) ethernet_priv;
}

void ethernet_close_handler(struct handler_t* handler) {
    struct ethernet_priv_t* private = (struct ethernet_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

bool ethernet_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct ethernet_write_args_t* ethernet_args = (struct ethernet_write_args_t*) packet_stack->args[packet_stack->stack_idx];    

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    struct ethernet_header_t* response_header = (struct ethernet_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct ethernet_header_t)); 

    if(out_buffer->offset < sizeof(struct ethernet_header_t)) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for ethernet header in buffer\n");         
        return false;   
    }

    memcpy(response_header->destination, ethernet_args->destination, ETHERNET_MAC_SIZE);
    memcpy(response_header->source, interface->mac, ETHERNET_MAC_SIZE);
    response_header->ethernet_type = ethernet_args->ethernet_type;

    out_buffer->offset -= sizeof(struct ethernet_header_t);

    // if this is the bottom of the packet stack, then write to the interface
    if(!packet_stack->stack_idx) {
        handler->handler_config->write(out_buffer, interface, 0);
    } else {
        const struct handler_t* next_handler = packet_stack->handlers[--packet_stack->stack_idx];
        return next_handler->operations.write(packet_stack, interface, next_handler);        
    }

    return true;
}

uint16_t ethernet_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {   
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    struct ethernet_header_t* header = (struct ethernet_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    if(header->ethernet_type > ntohs(0x0600) /* PLEASE OPTIMIZE */) {
        struct handler_t* next_handler = GET_FROM_PRIORITY(
            &ethernet_type_to_handler,
            header->ethernet_type, 
            struct handler_t
        );

        if(next_handler) {
            // set next buffer pointer for next protocol level     
            packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx] = ((uint8_t*) header) + sizeof(struct ethernet_header_t);
            
            struct ethernet_write_args_t ethernet_response_args;
            memcpy(&ethernet_response_args.destination, header->source, ETHERNET_MAC_SIZE);            
            ethernet_response_args.ethernet_type = header->ethernet_type;

            packet_stack->return_args[packet_idx] = &ethernet_response_args;

            next_handler->operations.read(packet_stack, interface, next_handler);            
        } else {
            NETSTACK_LOG(NETSTACK_WARNING, "Received non-supported ether_type: %hx\n", header->ethernet_type);            
        }        
    } else {
        NETSTACK_LOG(NETSTACK_WARNING, "Received IEEE 802.3 ethenet packet. This standard is not supported. ether_type: %hx\n", header->ethernet_type);        
    }    
}

struct handler_t* ethernet_create_handler(struct handler_config_t *handler_config) {    
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("ethernet handler", sizeof(struct handler_t)); 
    handler->handler_config = handler_config;

    handler->init = ethernet_init_handler;
    handler->close = ethernet_close_handler;

    handler->operations.write = ethernet_write;
    handler->operations.read = ethernet_read;

    return handler;
}