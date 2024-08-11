#include <stdlib.h>
#include <string.h>

#include "handlers/id.h"
#include "handlers/handler.h"

/*
 * Id Handler. Just responds immediately, does not write any data in the response. Is useful for testing
 * Specification: Custom, no RFC
 */



void id_close_handler(struct handler_t* handler) {}

void id_init_handler(struct handler_t* handler) {}


struct id_header_t* id_get_id_header(const uint8_t* data, const uint64_t size) {
    const uint16_t minimum_size = sizeof(struct id_header_t);

    if(size < minimum_size) {
        return 0;
    }

    struct id_header_t* id_header = (struct id_header_t*) ((data + size) - sizeof(struct id_header_t));

    if(id_header->magic_number == ID_MAGIC_NUMBER) {
        return id_header;
    }

    return 0;
}


uint16_t id_handle_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, struct interface_t* interface) {        
    struct id_header_t* response_header = (struct id_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    response_header->magic_number = ID_MAGIC_NUMBER;
    response_header->num_bytes_before = response_buffer->offset;

    uint16_t num_bytes_written = sizeof(struct id_header_t);
    response_buffer->offset += num_bytes_written;
    
    return num_bytes_written;
}

uint16_t id_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->write_chain_length++;
    packet_stack->response[packet_idx] = id_handle_response;
    handler_response(packet_stack, interface);
}


struct handler_t* id_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("id handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = id_init_handler;
    handler->close = id_close_handler;

    handler->operations.read = id_read;

    return handler;
}


