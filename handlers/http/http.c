#include "handlers/http/http.h"

#include "util/log.h"


/*
 * Transport Layer Security (http).
 * Specification: RFC 8446
 * 
 */



void http_close_handler(struct handler_t* handler) {
    struct http_priv_t* private = (struct http_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void http_init_handler(struct handler_t* handler, void* priv_config) {
    struct http_priv_t* ipv4_priv = (struct http_priv_t*) NET_STACK_MALLOC("http handler private data", sizeof(struct http_priv_t)); 
    handler->priv = (void*) ipv4_priv;
}


bool http_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {

}

uint16_t http_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx++;
    packet_stack->handlers[packet_idx] = handler;  
    uint8_t* packet = (uint8_t*) packet_stack->in_buffer.packet_pointers[packet_idx];

    switch

  
}


struct handler_t* http_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("http handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = http_init_handler;
    handler->close = http_close_handler;

    handler->operations.read = http_read;
    handler->operations.write = http_write;

    return handler;
}