#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include "test/tcp/tests/active_handshake/active_handshake.h"
#include "test/tcp/tests/active_handshake/active_handshake_packages.h"
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
bool connected = false;

bool nothing(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    tcp_response_buffer = &packet_stack->out_buffer;
    return true;
}

struct tcp_header_t* test_get_tcp_package(struct out_buffer_t* buffer) {
    return (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
}

struct in_packet_stack_t create_in_packet_stack(const void* package, struct handler_t* ip_handler) {
    struct in_packet_stack_t packet_stack = { 
        .in_buffer = { .packet_pointers = get_ipv4_header_from_package(package) }, // we need this extra layer because tcp depends on ip
        .stack_idx = 1, .handlers = ip_handler };

    packet_stack.in_buffer.packet_pointers[1] = get_tcp_header_from_package(package);
    packet_stack.in_buffer.packet_pointers[2] = 0;
    
    return packet_stack;
}


void connect() {
    connected = true;
}

void receive(uint8_t *data, uint64_t size) {
    // DUMMY, Not called in this test
}

bool tcp_test_active_handshake(struct handler_t* handler, struct test_config_t* config) {
    struct ipv4_header_t* ipv4_first_header = get_ipv4_header_from_package(pkt1);
    struct tcp_header_t* tcp_first_header = get_tcp_header_from_package(pkt1);
    struct tcp_header_t* tcp_second_header = get_tcp_header_from_package(pkt2);

    // mock write callbacks
    handler->handler_config->write = 0;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = 1; 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = 1024; // change for whatever is in the dump

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
    
      
    // set up tcp socket 
    struct tcp_socket_t* tcp_socket = tcp_create_socket(0, tcp_first_header->source_port, ipv4_first_header->source_ip, receive); 

    tcp_socket->socket.handlers[0] = ip_handler; // ipv4
    tcp_socket->socket.handlers[1] = handler; // tcp

    struct ipv4_write_args_t ipv4_args = {
        .destination_ip = ipv4_first_header->destination_ip,
        .protocol = 0x06       
    };

    tcp_socket->socket.handler_args[0] = &ipv4_args;
    // no need for tcp, because those args will be set by the connect call to the tcp socket

    tcp_socket->socket.depth = 2;

    tcp_add_socket(handler, tcp_socket);

    /******************************* TESTING START ****************************/

    // 1st (SYN)
    uint32_t connection_id = tcp_socket->operations.connect(handler, tcp_socket, 
        ipv4_first_header->destination_ip, 
        tcp_first_header->destination_port, 
        connect);

    if(!connection_id) {
        return false;
    }

    struct tcp_header_t* expected_tcp_header = get_tcp_header_from_package(pkt1);
    struct tcp_header_t* tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    
    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores)) {
        return false;
    }


    // 2nd (SYN-ACK, ACK)
    struct in_packet_stack_t packet_stack = packet_stack = create_in_packet_stack(pkt2, ip_handler);
        
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }   

    expected_tcp_header = get_tcp_header_from_package(pkt3);
    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) || !connected) {
        return false;
    }

    return true;
}