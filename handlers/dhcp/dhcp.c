#include "handlers/dhcp/dhcp.h"

#include "util/log.h"


/*
 * Dynamic Host Configuration Protocol (DHCP).
 * Specification: RFC 2131
 * 
 */


      
void dhcp_close_handler(struct handler_t* handler) {
    struct dhcp_priv_t* private = (struct dhcp_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void dhcp_init_handler(struct handler_t* handler, void* priv_config) {
    struct dhcp_priv_t* dhcp_priv = (struct dhcp_priv_t*) NET_STACK_MALLOC("dhcp handler private data", sizeof(struct dhcp_priv_t)); 
    handler->priv = (void*) dhcp_priv;
}


bool dhcp_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
}

uint16_t dhcp_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    struct dhcp_header_t* packet = (struct dhcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    if(packet->op != DHCP_OPCODE_BOOTREQUEST) {        
        // TODO: implement DHCP server functionality
    } else if(packet->op != DHCP_OPCODE_BOOTREPLY) {        
        // TODO: implement DHCP client functionality
    } else {
        return 3;
    }

    return 0;
}


struct handler_t* dhcp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("dhcp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = dhcp_init_handler;
    handler->close = dhcp_close_handler;

    handler->operations.read = dhcp_read;
    handler->operations.write = dhcp_write;

    return handler;
}