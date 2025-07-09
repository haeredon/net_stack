#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include "test/tcp/tests/download_1/download_1.h"
#include "test/tcp/tests/download_1/download_1_packages.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/socket.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/custom/custom.h"
#include "handlers/ethernet/ethernet.h"
#include "test/tcp/utility.h"
#include "util/array.h"

struct out_buffer_t* tcp_response_buffer;
uint8_t out_buffer[2048]; // used for writing

bool nothing(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    tcp_response_buffer = &packet_stack->out_buffer;
    return true;
}

uint16_t test_get_tcp_header_length(struct tcp_header_t* header) {
    return ((header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;    
}

struct tcp_header_t* test_get_tcp_package(struct out_buffer_t* buffer) {
    return (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
}

uint16_t test_get_tcp_package_payload_length(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return tcp_response_buffer->size - tcp_response_buffer->offset - get_tcp_header_length(header);
}

void* test_get_tcp_package_payload(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return (uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset + get_tcp_header_length(header);  
}

struct in_packet_stack_t create_in_packet_stack(const void* package, struct handler_t* ip_handler) {
    struct in_packet_stack_t packet_stack = { 
        .in_buffer = { .packet_pointers = get_ipv4_header_from_package(package) }, // we need this extra layer because tcp depends on ip
        .stack_idx = 1, .handlers = ip_handler };

    packet_stack.in_buffer.packet_pointers[1] = get_tcp_header_from_package(package);
    packet_stack.in_buffer.packet_pointers[2] = 0;
    
    return packet_stack;
}

struct out_packet_stack_t create_out_packet_stack(struct handler_t* ip_handler, 
    struct handler_t* tcp_handler, struct tcp_write_args_t* tcp_args) {
    struct out_packet_stack_t out_packet_stack = {
        .handlers = { ip_handler, tcp_handler },        
        .out_buffer = {
            .buffer = &out_buffer,
            .offset = 2048,
            .size = 2048    
        },
        .stack_idx = 1        
    };
    out_packet_stack.args[1] = tcp_args;

    return out_packet_stack;
}



void receive(uint8_t *data, uint64_t size) {

}

bool tcp_test_download_1(struct handler_t* handler, struct test_config_t* config) {
    // mock write callbacks
    handler->handler_config->write = 0;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = 1; 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = 1024; 

    // mock ip lower level and uppler level handlers write functions
    struct handler_config_t handler_config = {
        .mem_allocate = handler->handler_config->mem_allocate,
        .mem_free = handler->handler_config->mem_free,
        .write = 0 
    };

    struct ipv4_priv_t ipv4_priv = {
        .dummy = 42
    };

    struct handler_t* ip_handler = ipv4_create_handler(&handler_config); 
    ip_handler->init(ip_handler, &ipv4_priv);       
    ip_handler->operations.write = nothing;
    
    // 1st    
    struct tcp_socket_t socket = tcp_create_socket(0, 1337, 1232132, receive);
    tcp_add_socket(handler, &socket);

    socket.operations.open(socket);

    

    return true;
}