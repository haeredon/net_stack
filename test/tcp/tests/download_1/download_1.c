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

struct package_buffer_t* tcp_response_buffer;

bool nothing(struct packet_stack_t* packet_stack, struct package_buffer_t* buffer, uint8_t stack_idx, struct interface_t* interface, const struct handler_t* handler) {
    tcp_response_buffer = buffer;
    return true;
}

uint16_t test_get_tcp_header_length(struct tcp_header_t* header) {
    return ((header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;    
}

struct tcp_header_t* test_get_tcp_package(struct package_buffer_t* buffer) {
    return (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->data_offset);
}

uint16_t test_get_tcp_package_payload_length(struct package_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->data_offset);
    return tcp_response_buffer->size - tcp_response_buffer->data_offset - get_tcp_header_length(header);
}

void* test_get_tcp_package_payload(struct package_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->data_offset);
    return (uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->data_offset + get_tcp_header_length(header);  
}

struct packet_stack_t create_packet_stack(const void* header, struct handler_t* ip_handler) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = get_ipv4_header_from_package(header), // we need this extra layer because tcp depends on ip
        .write_chain_length = 1, .handlers = ip_handler };

    packet_stack.packet_pointers[1] = get_tcp_header_from_package(header);
    packet_stack.packet_pointers[2] = 0;
    
    return packet_stack;
}

bool tcp_test_download_1(struct handler_t* handler, struct test_config_t* config) {
    struct ipv4_header_t* ipv4_first_header = get_ipv4_header_from_package(pkt37);
    struct tcp_header_t* tcp_first_header = get_tcp_header_from_package(pkt37);
    struct tcp_header_t* tcp_second_header = get_tcp_header_from_package(pkt38);

    // mock write callbacks
    handler->handler_config->write = 0;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = ntohl(tcp_second_header->sequence_num); 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = ntohs(tcp_second_header->window); 

    // mock ip lower level and uppler level handlers write functions
    struct handler_config_t handler_config = {
        .mem_allocate = handler->handler_config->mem_allocate,
        .mem_free = handler->handler_config->mem_free,
        .write = 0 
    };

    struct ipv4_priv_t ipv4_priv = {
        .dummy = 42
    };

    struct handler_t* custom_handler = custom_create_handler(&handler_config);
    struct handler_t* ip_handler = ipv4_create_handler(&handler_config); 
    ip_handler->init(ip_handler, &ipv4_priv);       
    custom_handler->init(custom_handler, 0);

    ip_handler->operations.write = nothing;

    // add mock socket
    struct tcp_socket_t socket = {
        .ipv4 = ipv4_first_header->destination_ip,
        .listening_port = tcp_first_header->destination_port,
        .trans_control_block = 0
    };
    socket.next_handler = custom_handler;
    tcp_add_socket(handler, &socket);
    

    // FIRST (SYN, SYN-ACK)
    struct packet_stack_t packet_stack = create_packet_stack(pkt37, ip_handler);
    struct tcp_header_t* expected_tcp_header = get_tcp_header_from_package(pkt38);
    void* expected_tcp_payload = get_tcp_payload_payload_from_package(pkt38);
    uint16_t expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt38);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    struct tcp_header_t* tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    uint16_t actual_tcp_payload_length = test_get_tcp_package_payload_length(tcp_response_buffer);
    void* actual_tcp_payload = test_get_tcp_package_payload(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) ||
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    // SECOND (ACK)
    packet_stack = create_packet_stack(pkt39, ip_handler);
    // no reponse for this one, so we empty the buffer and check if it stays empty

    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // THIRD (ACK-PSH, ACK)
    packet_stack = create_packet_stack(pkt40, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt41);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt41);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt41);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    actual_tcp_payload_length = test_get_tcp_package_payload_length(tcp_response_buffer);
    actual_tcp_payload = test_get_tcp_package_payload(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) ||
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    return true;
}