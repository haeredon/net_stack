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

uint8_t tcp_header_buffer[4096];
uint8_t tcp_payload_buffer[4096];

struct response_buffer_t header_buffer;
struct response_buffer_t payload_buffer;

uint16_t tcp_header_write(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config) {
    header_buffer.buffer = tcp_header_buffer;
    header_buffer.offset = 0;
    header_buffer.size = 4096;
    header_buffer.stack_idx = 1;
    
    packet_stack->pre_build_response[1](packet_stack, &header_buffer, interface);
}

uint16_t tcp_payload_write(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config) {
    payload_buffer.buffer = tcp_payload_buffer;
    payload_buffer.offset = 0;
    payload_buffer.size = 4096;
    payload_buffer.stack_idx = 2;
    
    packet_stack->pre_build_response[2](packet_stack, &payload_buffer, interface);
}


uint16_t write_payload() {
    
}

struct packet_stack_t create_packet_stack(const void* header) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = get_ipv4_header_from_package(header), // we need this extra layer because tcp depends on ip
        .write_chain_length = 1 };

    packet_stack.packet_pointers[1] = get_tcp_header_from_package(header);
    packet_stack.packet_pointers[2] = 0;
    
    return packet_stack;
}

bool tcp_test_download_1(struct handler_t* handler, struct test_config_t* config) {
    struct ethernet_header_t* ethernet_first_header = (struct ethernet_header_t*) pkt37;
    struct ipv4_header_t* ipv4_first_header = get_ipv4_header_from_package(pkt37);
    struct tcp_header_t* tcp_first_header = get_tcp_header_from_package(pkt37);
    struct tcp_header_t* tcp_second_header = get_tcp_header_from_package(pkt38);

    // mock write callbacks
    handler->handler_config->write = tcp_header_write;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = ntohl(tcp_second_header->sequence_num); 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = ntohs(tcp_second_header->window); 

    // next level handler to receive§ tcp packages
    struct handler_config_t handler_config = {
        .mem_allocate = handler->handler_config->mem_allocate,
        .mem_free = handler->handler_config->mem_free,
        .write = tcp_payload_write 
    };
    struct handler_t* custom_handler = custom_create_handler(&handler_config);
    
    custom_handler->init(custom_handler, 0);

    // add mock socket
    struct tcp_socket_t socket = {
        .ipv4 = ipv4_first_header->destination_ip,
        .listening_port = tcp_first_header->destination_port,
        .trans_control_block = 0
    };
    socket.next_handler = custom_handler;
    tcp_add_socket(handler, &socket);
    

    // FIRST (SYN, SYN-ACK)
    struct packet_stack_t packet_stack = create_packet_stack(pkt37);
    struct tcp_header_t* expected_tcp_header = get_tcp_header_from_package(pkt38);
    void* expected_tcp_payload = get_tcp_payload_payload_from_package(pkt38);
    uint16_t expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt38);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    uint16_t actual_tcp_payload_length = payload_buffer.offset;
    void* actual_tcp_payload = payload_buffer.buffer;

    if(!is_tcp_header_equal((struct tcp_header_t*) tcp_header_buffer, expected_tcp_header, &ignores) ||
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    // // SECOND (ACK)
    packet_stack = create_packet_stack(pkt39);
    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_header_buffer, 0 , sizeof(tcp_header_buffer));
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_header_buffer, 4096)) {
        return false;
    }

    // // THIRD (ACK-PSH, ACK)
    packet_stack = create_packet_stack(pkt40);
    expected_tcp_header = get_tcp_header_from_package(pkt41);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt41);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt41);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    actual_tcp_payload_length = payload_buffer.offset;
    actual_tcp_payload = payload_buffer.buffer;

    if(!is_tcp_header_equal((struct tcp_header_t*) tcp_header_buffer, expected_tcp_header, &ignores) ||
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    return true;
}