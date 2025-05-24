#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"
#include "handlers/custom/custom.h"
#include "handlers/handler.h"

/*
 * Custom Handler. Just responds immediately with whatever data has been predefined for it. If no data has been defined it will write 
 * nothing in the response. Is useful for testing
 * Specification: Custom, no RFC
 */


void custom_close_handler(struct handler_t* handler) {
    struct custom_priv_t* private = (struct custom_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void custom_init_handler(struct handler_t* handler, void* priv_config) {
    struct custom_priv_t* private = (struct custom_priv_t*) handler->handler_config->mem_allocate("custom handler private data", sizeof(struct custom_priv_t)); 

    private->response_length = 0;
    private->response_buffer = 0;

    handler->priv = private;
}


uint16_t custom_handle_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface) {        
    struct custom_priv_t* private = (struct custom_priv_t*) packet_stack->handlers[response_buffer->stack_idx]->priv;    
    uint8_t* buffer = ((uint8_t*) (response_buffer->buffer)) + response_buffer->offset;

    memcpy(buffer, private->response_buffer, private->response_length);
    response_buffer->offset += private->response_length;
    return private->response_length;
}


bool write(struct packet_stack_t* packet_stack, struct package_buffer_t* buffer, uint8_t stack_idx, struct interface_t* interface, const struct handler_t* handler) {
    struct custom_priv_t* private = (struct custom_priv_t*) packet_stack->handlers[stack_idx]->priv;    
    uint8_t* response_buffer = (uint8_t*) buffer->buffer + buffer->data_offset - private->response_length;

    if(buffer->data_offset < private->response_length) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for custom header in buffer\n");         
        return false;   
    };

    memcpy(response_buffer, private->response_buffer, private->response_length);
    buffer->data_offset -= private->response_length;
   
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
    packet_stack->handlers[packet_stack->write_chain_length] = handler;      
    handler->operations->write(packet_stack, /* need to allocate buffer here */, interface);
}

void custom_set_response(struct handler_t* handler, void* response_buffer, uint32_t response_length) {
    struct custom_priv_t* private = (struct custom_priv_t*) handler->priv;    
    private->response_buffer = response_buffer;
    private->response_length = response_length;
}

struct handler_t* custom_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("null handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = custom_init_handler;
    handler->close = custom_close_handler;

    handler->operations.read = read;
    handler->operations.write = write;

    return handler;
}


