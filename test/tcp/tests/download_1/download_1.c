#include <stdbool.h>

#include "download_1.h"
#include "download_1_packages.h"
#include "tcp/utility.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/ethernet/ethernet.h"


const uint16_t BUFFER_SIZE = 4096;
uint8_t response_buffer[BUFFER_SIZE];

uint16_t write(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config) {
    
}

uint16_t get_ip_header(void* header) {
    return header + IP_HEADER_OFFSET;
}

uint16_t get_tcp_header(void* header) {
    return header + TCP_HEADER_OFFSET;
}

struct packet_stack_t create_packet_stack(void* header) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = get_tcp_header(header), // we need this extra layer because tcp depends on ip
        .write_chain_length = 1 };

    packet_stack.packet_pointers[1] = get_tcp_header_offset(header);
    
    return packet_stack;
}

bool tcp_test_download_1(struct handler_t* handler, struct test_config_t* config) {
    struct ethernet_header_t* ethernet_first_header = (struct ethernet_header_t*) pkt37;
    struct ipv4_header_t* ipv4_first_header = (struct ipv4_header_t*) get_ip_header(pkt37);
    
    // set interface according to first header
    config->interface->ipv4_addr = ipv4_first_header->destination_ip;    
    memcpy(config->interface, ethernet_first_header->destination, ETHERNET_MAC_SIZE);

    // mock write callbacks
    handler->handler_config->write = write;

    // FIRST
    struct packet_stack_t packet_stack = create_packet_stack(pkt37);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!is_tcp_packet_equal((struct tcp_header_t*) response_buffer, 
        (struct tcp_header_t*) (get_tcp_header(pkt38)), &ignores)) {
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