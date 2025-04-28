#include <stdlib.h>
#include <string.h>

#include "handlers/null/null.h"
#include "handlers/handler.h"

/*
 * Id Handler. Just responds immediately, does not write any data in the response. Is useful for testing
 * Specification: Custom, no RFC
 */



void null_close_handler(struct handler_t* handler) {}

void null_init_handler(struct handler_t* handler, void* priv_config) {}


struct null_header_t* null_get_null_header(const uint8_t* data, const uint64_t size) {
    const uint16_t minimum_size = sizeof(struct null_header_t);

    if(size < minimum_size) {
        return 0;
    }

    struct null_header_t* null_header = (struct null_header_t*) ((data + size) - sizeof(struct null_header_t));

    if(null_header->magic_number == ID_MAGIC_NUMBER) {
        return null_header;
    }

    return 0;
}


uint16_t null_handle_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface) {        
    return 0;
}

uint16_t null_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->write_chain_length++;
    packet_stack->pre_build_response[packet_idx] = null_handle_response;
    handler_response(packet_stack, interface, 0);
}


struct handler_t* null_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("null handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = null_init_handler;
    handler->close = null_close_handler;

    handler->operations.read = null_read;

    return handler;
}


