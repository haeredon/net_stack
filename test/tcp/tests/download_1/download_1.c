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
#include "test/utility.h"
#include "test/tcp/utility.h"
#include "util/array.h"

static struct out_buffer_t* tcp_response_buffer;
static uint8_t out_buffer[2048]; // used for writing

static bool nothing(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    tcp_response_buffer = &packet_stack->out_buffer;
    return true;
}

static uint16_t test_get_tcp_header_length(struct tcp_header_t* header) {
    return ((header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;    
}

static struct tcp_header_t* test_get_tcp_package(struct out_buffer_t* buffer) {
    return (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
}

static uint16_t test_get_tcp_package_payload_length(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return tcp_response_buffer->size - tcp_response_buffer->offset - get_tcp_header_length(header);
}

static void* test_get_tcp_package_payload(struct out_buffer_t* buffer) {
    struct tcp_header_t* header = (struct tcp_header_t*) ((uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset);
    return (uint8_t*) tcp_response_buffer->buffer + tcp_response_buffer->offset + get_tcp_header_length(header);  
}

static struct in_packet_stack_t create_in_packet_stack(const void* package, struct handler_t* ip_handler) {
    struct in_packet_stack_t packet_stack = { 
        .in_buffer = { .packet_pointers = get_ipv4_header_from_package(package) }, // we need this extra layer because tcp depends on ip
        .stack_idx = 1, .handlers = ip_handler };

    packet_stack.in_buffer.packet_pointers[1] = get_tcp_header_from_package(package);
    packet_stack.in_buffer.packet_pointers[2] = 0;
    
    return packet_stack;
}

static struct out_packet_stack_t create_out_packet_stack(struct handler_t* ip_handler, 
        struct handler_t* tcp_handler, struct handler_t* custom_handler, struct tcp_write_args_t* tcp_args) {
    struct out_packet_stack_t out_packet_stack = {
        .handlers = { ip_handler, tcp_handler, custom_handler },        
        .out_buffer = {
            .buffer = &out_buffer,
            .offset = 2048,
            .size = 2048    
        },
        .stack_idx = 2        
    };
    out_packet_stack.args[1] = tcp_args;

    return out_packet_stack;
}


static void connect_callback() {}

static void on_close(uint8_t *data, uint64_t size) {}

static void receive(uint8_t *data, uint64_t size) {}

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
        .mem_allocate = NET_STACK_MALLOC,
        .mem_free = NET_STACK_FREE,
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
    // set up tcp socket 
    struct tcp_socket_t* tcp_socket = tcp_create_socket(0, tcp_first_header->destination_port, ipv4_first_header->destination_ip, receive, connect_callback, on_close); 
    tcp_socket->next_handler = custom_handler;
    tcp_add_socket(handler, tcp_socket);

    // add arguments for socket writes
    struct tcp_write_args_t tcp_args = {
        .connection_id = tcp_shared_calculate_connection_id(ipv4_first_header->source_ip, tcp_first_header->source_port, tcp_first_header->destination_port),
        .socket = tcp_socket,
        .flags = TCP_ACK_FLAG | TCP_PSH_FLAG
    };

    // FIRST (SYN, SYN-ACK)
    struct in_packet_stack_t packet_stack = create_in_packet_stack(pkt37, ip_handler);
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
    packet_stack = create_in_packet_stack(pkt39, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // THIRD (ACK-PSH (client hello), ACK, ACK-PSH (server hello)). We only check the correctness of the server_hello here
    packet_stack = create_in_packet_stack(pkt40, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt42);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt42);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt42);
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

    // FOURTH (ACK)
    packet_stack = create_in_packet_stack(pkt43, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // FIFTH (ACK-PSH (Change cipher)
    struct out_packet_stack_t out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt44);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt44);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt44);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // SIXTH (ACK)
    packet_stack = create_in_packet_stack(pkt45, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // SEVENTH (ACK-PSH)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt46);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt46);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt46);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // EIGHT (ACK)
    packet_stack = create_in_packet_stack(pkt47, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // NINTH (ACK-PSH)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt48);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt48);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt48);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // TENTH (ACK)
    packet_stack = create_in_packet_stack(pkt49, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // ELEVENTH (ACK-PSH)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt50);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt50);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt50);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // 12TH (ACK)
    packet_stack = create_in_packet_stack(pkt51, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // 13TH (ACK-PSH) PCAPNH(17)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt53);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt53);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt53);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // 14TH (ACK-PSH (change_cipher), ACK-PSH) PCAPNH(16)
    packet_stack = create_in_packet_stack(pkt52, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt54);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt54);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt54);
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

    // 15TH (ACK-PSH, ACK) PCAPNH(19)
    packet_stack = create_in_packet_stack(pkt55, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt56);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt56);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt56);
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

    // 16TH (ACK-PSH, ACK) PCAPNH(21)
    packet_stack = create_in_packet_stack(pkt58, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt59);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt59);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt59);
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

    // 17TH (ACK-PSH) PCAPNH(23)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt60);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt60);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt60);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // 18TH (ACK-PSH) PCAPNH(24)
    out_packet_stack = create_out_packet_stack(ip_handler, handler, custom_handler, &tcp_args);

    expected_tcp_header = get_tcp_header_from_package(pkt61);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt61);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt61);
    custom_set_response(custom_handler, expected_tcp_payload, expected_tcp_payload_length);
    
    if(!custom_handler->operations.write(&out_packet_stack, config->interface, custom_handler)) {
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

    // 19TH (ACK-PSH) PCAPNH(25)
    packet_stack = create_in_packet_stack(pkt62, ip_handler);

    // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(tcp_response_buffer->buffer, 0 , tcp_response_buffer->size);
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(!array_is_zero(tcp_response_buffer->buffer, tcp_response_buffer->size)) {
        return false;
    }

    // 20TH (FIN-ACK, FIN-ACK) PCAPNH(26)
    packet_stack = create_in_packet_stack(pkt63, ip_handler);
    expected_tcp_header = get_tcp_header_from_package(pkt64);
    expected_tcp_payload = get_tcp_payload_payload_from_package(pkt64);
    expected_tcp_payload_length = get_tcp_payload_length_from_package(pkt64);
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

    // 21TH (ACK) PCAPNH(28)
    packet_stack = create_in_packet_stack(pkt65, ip_handler);

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