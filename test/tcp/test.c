#include "handlers/ipv4.h"
#include "handlers/tcp.h"
#include "handlers/ethernet.h"
#include "test.h"
#include "test/common.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>

/*************************************
 *          TEST DATA                *
**************************************/

/* Frame (74 bytes) */
static const unsigned char tcp_3_way_handshake_1[60] = {
    0x45, 0x00, 0x00, 0x3c, 0x27, 0xe8, 0x40, 0x00, 
    0x40, 0x06, 0x08, 0x7a, 0xc0, 0xa8, 0x00, 0x75, 
    0x20, 0x20, 0x20, 0x20, 0xde, 0xc2, 0x01, 0xbb,
    0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 
    0xa0, 0x02, 0xfa, 0xf0, 0x11, 0x11, 0x00, 0x00, 
    0x02, 0x04, 0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 
    0x22, 0xe6, 0x59, 0xfa, 0x00, 0x00, 0x00, 0x00, 
    0x01, 0x03, 0x03, 0x07                                      
};

/* Frame (74 bytes) */
static const unsigned char tcp_3_way_handshake_2[60] = {
    0x45, 0x80, 0x00, 0x3c, 0x00, 0x00, 0x40, 0x00, 
    0x7a, 0x06, 0xf5, 0xe1, 0x20, 0x20, 0x20, 0x20, 
    0xc0, 0xa8, 0x00, 0x75, 0x01, 0xbb, 0xde, 0xc2, 
    0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x0b, 
    0xa0, 0x12, 0xff, 0xff, 0x11, 0x11, 0x00, 0x00, 
    0x02, 0x04, 0x05, 0x84, 0x04, 0x02, 0x08, 0x0a, 
    0x88, 0x41, 0x03, 0x0f, 0x22, 0xe6, 0x59, 0xfa, 
    0x01, 0x03, 0x03, 0x08                                      
};

/* Frame (66 bytes) */
static const unsigned char tcp_3_way_handshake_3[60] = {
    0x45, 0x00, 0x00, 0x34, 0x27, 0xe9, 0x40, 0x00, 
    0x40, 0x06, 0x08, 0x81, 0xc0, 0xa8, 0x00, 0x75,
    0x20, 0x20, 0x20, 0x20, 0xde, 0xc2, 0x01, 0xbb, 
    0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0xaa, 0x01, 
    0x80, 0x10, 0x01, 0xf6, 0x11, 0x11, 0x00, 0x00, 
    0x01, 0x01, 0x08, 0x0a, 0x22, 0xe6, 0x5a, 0x26, 
    0x88, 0x41, 0x03, 0x0f                                      
};


/*************************************
 *          UTILITY                  *
**************************************/
struct response_t tcp_last_response;
uint8_t tcp_response_buffer[4096];

int64_t tcp_test_write(struct response_t response) {
    memcpy(&tcp_last_response, &response, sizeof(struct response_t));
    memcpy(&tcp_response_buffer, response.buffer, sizeof(tcp_response_buffer));
    tcp_last_response.buffer = tcp_response_buffer;
}

bool tcp_send_and_check_response(const uint8_t* request, const uint8_t* response, uint16_t response_size, struct handler_t* handler, struct test_config_t* config) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = request, .write_chain_length = 1 };

    packet_stack.packet_pointers[1] = request + 20;
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(tcp_last_response.size != response_size ||
       memcmp(tcp_response_buffer, response, tcp_last_response.size)) {
        return false;
    }

    return true;
}

/*************************************
 *          TESTS                *
**************************************/
bool tcp_test_basic(struct handler_t* handler, struct test_config_t* config) {
    return tcp_send_and_check_response(tcp_3_way_handshake_1, tcp_3_way_handshake_2, sizeof(tcp_3_way_handshake_2), handler, config);
}

/*************************************
 *          BOOTSTRAP                *
**************************************/
bool tcp_tests_start() {        
    struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free 
    };	

    struct interface_t interface = {
        .port = 0,
        .port = 0,
        .operations.write = tcp_test_write,
        .ipv4_addr = OWN_IP,
        .mac = 0
    };
    memcpy(&interface, OWN_MAC, ETHERNET_MAC_SIZE);

    struct test_config_t test_config = { 
        .interface = &interface
    };	

	struct handler_t* tcp_handler = tcp_create_handler(&handler_config); 
	tcp_handler->init(tcp_handler);

    struct tcp_socket_t* socket = (struct tcp_socket_t*) malloc(sizeof(struct tcp_socket_t));
    socket->listening_port = htons(443);
    socket->interface = &interface;
    tcp_add_socket(socket, tcp_handler);

    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(test->name, "3-Way Handshake", sizeof("3-Way Handshake"));    
    test->test = tcp_test_basic;

    /*
     * need test for 
     *      edge cases on receive window
     *      edge cases for ack vs seq
     *      teardown correctly on error
     *      reacts correctly to fin and rst
     *      checksum validation
     *      close connection if outbound buffer full
     *      out and in buffer fills up
    */

    struct test_t* tests[2];
    tests[0] = test;
    tests[1] = 0; // zero out last pointer to mark end of array

    struct test_run_t ipv4_test_run = {
        .config = &test_config,
        .handler = tcp_handler,
        .tests = tests       
    };

    run_tests(&ipv4_test_run);
}



