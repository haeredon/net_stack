#include "handlers/llc/llc.h"

#include "handlers/protocol_map.h"
#include "util/log.h"


/*
 * Logical Link Control (LLC).
 * Specification: IEEE Std 802.2
 * 
 * Basic implementation only to support STP as the upper level protocol. 
 * 
 * 
 * Supports:
 *      Unumbered Format
 *      8 bit control only
 * 
 */

struct key_val_t llc_key_vals[LLC_NUM_PROTOCOL_TYPE_ENTRIES];
struct priority_map_t llc_type_to_handler = { 
    .max_size = LLC_NUM_PROTOCOL_TYPE_ENTRIES,
    .map = llc_key_vals
};


void llc_close_handler(struct handler_t* handler) {
    struct llc_priv_t* private = (struct llc_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void llc_init_handler(struct handler_t* handler, void* priv_config) {
    struct llc_priv_t* ipv4_priv = (struct llc_priv_t*) NET_STACK_MALLOC("ipv4 handler private data", sizeof(struct llc_priv_t)); 
    handler->priv = (void*) ipv4_priv;
}


bool llc_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct llc_write_args_t* llc_args = (struct llc_write_args_t*) packet_stack->args[packet_stack->stack_idx];    

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    struct llc_header_t* response_header = (struct llc_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct llc_header_t)); 

    if(out_buffer->offset < sizeof(struct llc_header_t)) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for llc header in buffer\n");         
        return false;   
    };

    out_buffer->offset -= sizeof(struct llc_header_t);

    response_header->destination_access_service_point = llc_args->destination_access_service_point; 
    response_header->source_access_service_point = llc_args->source_access_service_point; 
    response_header->control = llc_args->control;

    // if this is the bottom of the packet stack, then write to the interface
    if(!packet_stack->stack_idx) {
        handler->handler_config->write(out_buffer, interface, 0);
    } else {
        const struct handler_t* next_handler = packet_stack->handlers[--packet_stack->stack_idx];
        return next_handler->operations.write(packet_stack, interface, next_handler);        
    }
    
    return true;
}

uint16_t llc_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    struct llc_header_t* header = (struct llc_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    if(header->control != LLC_UNUMBERED_FORMAT) {
        NETSTACK_LOG(NETSTACK_INFO, "Only ununmbered format is supported\n");          
        return 1;
    }

    struct handler_t* next_handler = GET_FROM_PRIORITY(
        &llc_type_to_handler,
        header->control, 
        struct handler_t
    );

    if(next_handler) {
        // set next buffer pointer for next protocol level     
        packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx] = ((uint8_t*) header) + sizeof(struct llc_header_t);

        struct llc_write_args_t llc_response_args = {
            .destination_access_service_point = header->source_access_service_point,
            .source_access_service_point = header->destination_access_service_point,
            .control = header->control
        };

        packet_stack->return_args[packet_idx] = &llc_response_args;        

        next_handler->operations.read(packet_stack, interface, next_handler);  
    } else {
        NETSTACK_LOG(NETSTACK_WARNING, "LLC received non-supported protocol type: %hx\n", header->protocol);            
        return 4;
    }
    
    return 0;
}


struct handler_t* llc_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("llc handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = llc_init_handler;
    handler->close = llc_close_handler;

    handler->operations.read = llc_read;
    handler->operations.write = llc_write;

    return handler;
}