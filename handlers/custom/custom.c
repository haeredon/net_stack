#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "util/log.h"
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

bool custom_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct custom_priv_t* private = (struct custom_priv_t*) packet_stack->handlers[packet_stack->stack_idx]->priv;    

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    uint8_t* response_buffer = (uint8_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - private->response_length);

    if(out_buffer->offset < private->response_length) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for custom header in buffer\n");         
        return false;   
    };

    memcpy(response_buffer, private->response_buffer, private->response_length);
    out_buffer->offset -= private->response_length;
   
    // if this is the bottom of the packet stack, then write to the interface
    if(!packet_stack->stack_idx) {
        handler->handler_config->write(out_buffer, interface, 0);
    } else {        
        const struct handler_t* next_handler = packet_stack->handlers[--packet_stack->stack_idx];
        return next_handler->operations.write(packet_stack, interface, next_handler);        
    }
    
    return true;
}

uint16_t custom_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    packet_stack->handlers[packet_stack->stack_idx] = handler;   

    struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) handler->handler_config->
            mem_allocate("response: tcp_package", DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct out_packet_stack_t)); 

    memcpy(out_package_stack->handlers, packet_stack->handlers, 10 * sizeof(struct handler_t*));
    memcpy(out_package_stack->args, packet_stack->return_args, 10 * sizeof(void*));

    out_package_stack->out_buffer.buffer = (uint8_t*) out_package_stack + sizeof(struct out_packet_stack_t);
    out_package_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_package_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;      

    out_package_stack->stack_idx = packet_stack->stack_idx;

    handler->operations.write(out_package_stack, interface, handler);
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

    handler->operations.read = custom_read;
    handler->operations.write = custom_write;

    return handler;
}


