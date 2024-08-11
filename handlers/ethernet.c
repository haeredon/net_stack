#include <string.h>
#include <arpa/inet.h>

#include "handlers/ethernet.h"
#include "handlers/protocol_map.h"
 

#include "log.h"


struct key_val_t key_vals[ETHERNET_NUM_ETH_TYPE_ENTRIES];
struct priority_map_t ethernet_type_to_handler = { 
    .max_size = ETHERNET_NUM_ETH_TYPE_ENTRIES,
    .map = key_vals
};


void ethernet_init_handler(struct handler_t* handler) {
    struct ethernet_priv_t* ethernet_priv = (struct ethernet_priv_t*) handler->handler_config->mem_allocate("pcap handler private data", sizeof(struct ethernet_priv_t)); 
    handler->priv = (void*) ethernet_priv;
}

void ethernet_close_handler(struct handler_t* handler) {
    struct ethernet_priv_t* private = (struct ethernet_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

uint16_t ethernet_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, struct interface_t* interface) {
    struct ethernet_header_t* request_header = (struct ethernet_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct ethernet_header_t* response_header = (struct ethernet_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    memcpy(response_header->destination, request_header->source, ETHERNET_MAC_SIZE);
    memcpy(response_header->source, request_header->destination, ETHERNET_MAC_SIZE);
    response_header->ethernet_type = request_header->ethernet_type;

    uint16_t num_bytes_written = sizeof(struct ethernet_header_t);
    response_buffer->offset += num_bytes_written;
    
    return num_bytes_written;
}

uint16_t ethernet_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {   
    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct ethernet_header_t* header = (struct ethernet_header_t*) packet_stack->packet_pointers[packet_idx];

    packet_stack->response[packet_idx] = ethernet_response;

    if(header->ethernet_type > ntohs(0x0600) /* PLEASE OPTIMIZE */) {
        struct handler_t* next_handler = GET_FROM_PRIORITY(
            &ethernet_type_to_handler,
            header->ethernet_type, 
            struct handler_t
        );

        if(next_handler) {
            // set next buffer pointer for next protocol level     
            packet_stack->packet_pointers[packet_stack->write_chain_length] = ((uint8_t*) header) + sizeof(struct ethernet_header_t);
            
            next_handler->operations.read(packet_stack, interface, next_handler);            
        } else {
            NETSTACK_LOG(NETSTACK_WARNING, "Received non-supported ether_type: %hx\n", header->ethernet_type);            
        }        
    } else {
        NETSTACK_LOG(NETSTACK_WARNING, "Received IEEE 802.3 ethenet packet. This standard is not supported. ether_type: %hx\n", header->ethernet_type);        
    }    
}

struct handler_t* ethernet_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("ethernet handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = ethernet_init_handler;
    handler->close = ethernet_close_handler;

    handler->operations.read = ethernet_read;

    return handler;
}