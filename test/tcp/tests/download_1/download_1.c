#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include "test/tcp/tests/download_1/download_1.h"
#include "test/tcp/tests/download_1/download_1_packages.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/socket.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/ethernet/ethernet.h"
#include "test/tcp/utility.h"


uint8_t tcp_response_buffer[4096];

uint16_t write(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config) {
    struct response_buffer_t packet_buffer = {
        .buffer = tcp_response_buffer,
        .offset = 0,
        .size = 4096,
        .stack_idx = 1
    };
    packet_stack->pre_build_response[1](packet_stack, &packet_buffer, interface);
}

struct ipv4_header_t* get_ip_header(const void* header) {
    return (struct ipv4_header_t*) (((uint8_t*) header) + IP_HEADER_OFFSET);
}

struct tcp_header_t* get_tcp_header(const void* header) {
    return (struct tcp_header_t*) (((uint8_t*) header) + TCP_HEADER_OFFSET);
}

struct packet_stack_t create_packet_stack(const void* header) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = get_ip_header(header), // we need this extra layer because tcp depends on ip
        .write_chain_length = 1 };

    packet_stack.packet_pointers[1] = get_tcp_header(header);
    
    return packet_stack;
}

bool tcp_test_download_1(struct handler_t* handler, struct test_config_t* config) {
    struct ethernet_header_t* ethernet_first_header = (struct ethernet_header_t*) pkt37;
    struct ipv4_header_t* ipv4_first_header = get_ip_header(pkt37);
    struct tcp_header_t* tcp_first_header = get_tcp_header(pkt37);
    struct tcp_header_t* tcp_second_header = get_tcp_header(pkt38);

    // mock write callbacks
    handler->handler_config->write = write;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = ntohl(tcp_second_header->sequence_num); 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = ntohs(tcp_second_header->window); 

    // add mock socket
    struct tcp_socket_t socket = {
        .ipv4 = ipv4_first_header->destination_ip,
        .listening_port = tcp_first_header->destination_port,
        .trans_control_block = 0
    };
    tcp_add_socket(handler, &socket);

    // FIRST
    struct packet_stack_t packet_stack = create_packet_stack(pkt37);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!is_tcp_packet_equal((struct tcp_header_t*) tcp_response_buffer, get_tcp_header(pkt38), &ignores)) {
        return false;
    }

    // SECOND
    packet_stack = create_packet_stack(pkt39);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!is_tcp_packet_equal((struct tcp_header_t*) tcp_response_buffer, get_tcp_header(pkt40), &ignores)) {
        return false;
    }

    // // SECOND
    // struct packet_stack_t second_stack = { 
    //     .pre_build_response = 0, .post_build_response = 0,
    //     .packet_pointers = tcp_3_way_handshake_3 + tcp_3_way_handshake_ip_offset, 
    //     .write_chain_length = 1 
    // };

    // second_stack.packet_pointers[1] = tcp_3_way_handshake_3 + tcp_3_way_handshake_tcp_offset;

    // if(handler->operations.read(&second_stack, config->interface, handler)) {
    //     return false;
    // }

    // // check that no response was send. 
    // // We confirm this by checking that the last reponce buffer is the same is the previous one
    // if(!is_tcp_packet_equal3((struct tcp_header_t*) tcp_response_buffer, 
    //     (struct tcp_header_t*) (tcp_3_way_handshake_2 + tcp_3_way_handshake_tcp_offset), &ignores)) {
    //     return false;
    // }

    // // THIRD
    // struct packet_stack_t third_stack = { 
    //     .pre_build_response = 0, .post_build_response = 0,
    //     .packet_pointers = tcp_3_way_handshake_4 + tcp_3_way_handshake_ip_offset,  
    //     .write_chain_length = 1 
    // };

    // third_stack.packet_pointers[1] = tcp_3_way_handshake_4 + tcp_3_way_handshake_tcp_offset;

    // if(handler->operations.read(&third_stack, config->interface, handler)) {
    //     return false;
    // }

    // // check that the correct reponse was send and that it is equal to tcp_3_way_handshake_5
    // if(!is_tcp_packet_equal3((struct tcp_header_t*) tcp_response_buffer, 
    //     (struct tcp_header_t*) (tcp_3_way_handshake_5 + tcp_3_way_handshake_tcp_offset), &ignores)) {
    //     return false;
    // }

    return true;
}