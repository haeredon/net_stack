#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include "test/tcp/tests/active_mode/active_mode.h"
#include "test/tcp/tests/active_mode/active_mode_packages.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/socket.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/custom/custom.h"
#include "handlers/ethernet/ethernet.h"
#include "test/utility.h"
#include "test/tcp/utility.h"
#include "util/array.h"
 
static struct out_buffer_t* tcp_response_buffer;
static uint8_t out_buffer[2048]; // used for writing
bool connected = false;

static bool nothing(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    tcp_response_buffer = &packet_stack->out_buffer;
    return true;
}

static uint16_t test_get_tcp_package_payload_length(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return tcp_response_buffer->size - tcp_response_buffer->offset - get_tcp_header_length(header);
}

static void* test_get_tcp_package_payload(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return (uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset + get_tcp_header_length(header);  
}

static struct tcp_header_t* test_get_tcp_package(struct out_buffer_t* buffer) {
    return (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
}

static struct in_packet_stack_t create_in_packet_stack(const void* package, struct handler_t* ip_handler) {
    struct in_packet_stack_t packet_stack = { 
        .in_buffer = { .packet_pointers = get_ipv4_header_from_package(package) }, // we need this extra layer because tcp depends on ip
        .stack_idx = 1, .handlers = ip_handler };

    packet_stack.in_buffer.packet_pointers[1] = get_tcp_header_from_package(package);
    packet_stack.in_buffer.packet_pointers[2] = 0;
    
    return packet_stack;
}

 
void connect_callback() {
    connected = true;
}

void on_close(uint8_t *data, uint64_t size) {
    // DUMMY, Not called in this test
}

void receive(uint8_t *data, uint64_t size) {
    // DUMMY, Not called in this test
}

bool tcp_test_active_mode(struct handler_t* handler, struct test_config_t* config) {
    struct ipv4_header_t* ipv4_first_header = get_ipv4_header_from_package(pkt1);
    struct tcp_header_t* tcp_first_header = get_tcp_header_from_package(pkt1);
    struct tcp_header_t* tcp_second_header = get_tcp_header_from_package(pkt2);

    // mock write callbacks
    handler->handler_config->write = 0;
    // generated sequence number must be whatever which is in the response to the first header
    current_sequence_number = ntohl(tcp_first_header->sequence_num); 
    // also get window from response package
    ((struct tcp_priv_t*) handler->priv)->window = 1024; // change for whatever is in the dump

    // mock ip lower level and uppler level handlers write functions
    struct handler_config_t handler_config = {
        .mem_allocate = NET_STACK_MALLOC,
        .mem_free = NET_STACK_FREE,
        .write = 0 
    };

    struct ipv4_priv_t ipv4_priv = {
        .dummy = 42
    };

    struct handler_t* ip_handler = ipv4_create_handler(&handler_config); 
    ip_handler->init(ip_handler, &ipv4_priv);       
    ip_handler->operations.write = nothing;
    
      
    // set up tcp socket 
    struct tcp_socket_t* tcp_socket = tcp_create_socket(0, tcp_first_header->source_port, ipv4_first_header->source_ip, receive, connect_callback, on_close); 

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
        tcp_first_header->source_port,
        ipv4_first_header->destination_ip, 
        tcp_first_header->destination_port);

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

    // 3rd (PSH-ACK, ACK)
    void* expected_tcp_payload = get_tcp_payload_payload_from_package(pkt4);
    uint16_t expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt4);

    if(!tcp_socket->operations.send(tcp_socket, connection_id, expected_tcp_payload, expected_tcp_payload_length)) {
        return false;
    }

    expected_tcp_header = get_tcp_header_from_package(pkt4);
    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    uint16_t actual_tcp_payload_length = test_get_tcp_package_payload_length(tcp_response_buffer);
    void* actual_tcp_payload = test_get_tcp_package_payload(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) || 
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    packet_stack = create_in_packet_stack(pkt5, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // 4th (PSH-ACK, ACK)
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt6);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt6);

    if(!tcp_socket->operations.send(tcp_socket, connection_id, expected_tcp_payload, expected_tcp_payload_length)) {
        return false;
    }

    expected_tcp_header = get_tcp_header_from_package(pkt6);
    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    actual_tcp_payload_length = test_get_tcp_package_payload_length(tcp_response_buffer);
    actual_tcp_payload = test_get_tcp_package_payload(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) || 
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    packet_stack = create_in_packet_stack(pkt7, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // 5th (PSH-ACK, ACK) (8,9)
    packet_stack = create_in_packet_stack(pkt8, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    expected_tcp_header = get_tcp_header_from_package(pkt9);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt9);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt9);
    
    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);
    actual_tcp_payload_length = test_get_tcp_package_payload_length(tcp_response_buffer);
    actual_tcp_payload = test_get_tcp_package_payload(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores) || 
       (expected_tcp_payload_length == actual_tcp_payload_length && 
       memcmp(expected_tcp_payload, actual_tcp_payload, expected_tcp_payload_length))) {
        return false;
    }

    // 6th (FIN-ACK, FIN-ACK)
    packet_stack = create_in_packet_stack(pkt10, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt11);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    tcp_header_buffer = test_get_tcp_package(tcp_response_buffer);

    if(!is_tcp_header_equal(tcp_header_buffer, expected_tcp_header, &ignores)) {
        return false;
    }

    // 7th (ACK)
    packet_stack = create_in_packet_stack(pkt12, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    return true;
}