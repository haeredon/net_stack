
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/tcp_tcb.h"
#include "handlers/tcp/tcp_block_buffer.h"
#include "handlers/ethernet/ethernet.h"

#include "test.h"
#include "test_data.h"
#include "test/common.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>


/*************************************
 *          UTILITY                  *
**************************************/
struct tcp_cmp_ignores_t {
    bool sequence_num;
    bool checksum; 
};

struct tcp_cmp_ignores_t ignores = {
    .sequence_num = true,
    .checksum = true
};
struct response_t tcp_last_response;
uint8_t tcp_response_buffer[4096];

int64_t tcp_test_write(struct response_t response) {
    memcpy(&tcp_last_response, &response, sizeof(struct response_t));
    memcpy(&tcp_response_buffer, response.buffer, sizeof(tcp_response_buffer));
    tcp_last_response.buffer = tcp_response_buffer;
}


bool is_tcp_packet_equal3(struct tcp_header_t* a, struct tcp_header_t* b, struct tcp_cmp_ignores_t* ignores) {
    return a->source_port == b->source_port &&
    a->destination_port == b->destination_port &&
    (a->sequence_num == b->sequence_num || ignores->sequence_num) &&
    a->acknowledgement_num == b->acknowledgement_num &&
    a->data_offset == b->data_offset &&
    a->control_bits == b->control_bits &&
    a->window == b->window &&
    (a->checksum == b->checksum || ignores->checksum) &&
    a->urgent_pointer == b->urgent_pointer;
}

// bool is_tcp_packet_equal2(struct tcp_header_t* a, struct tcp_header_t* b) {
//     return is_tcp_packet_equal3(a, b, false);
// }

/*************************************
 *          TESTS                *
**************************************/
bool tcp_test_basic(struct handler_t* handler, struct test_config_t* config) {
    struct packet_stack_t first_stack = { 
    .pre_build_response = 0, .post_build_response = 0,
    .packet_pointers = tcp_3_way_handshake_1 + tcp_3_way_handshake_ip_offset, .write_chain_length = 1 };

    first_stack.packet_pointers[1] = tcp_3_way_handshake_1 + tcp_3_way_handshake_tcp_offset;
    
    if(handler->operations.read(&first_stack, config->interface, handler)) {
        return false;
    }

    if(!is_tcp_packet_equal3((struct tcp_header_t*) tcp_response_buffer, 
        (struct tcp_header_t*) (tcp_3_way_handshake_2 + tcp_3_way_handshake_tcp_offset), &ignores)) {
        return false;
    }

    // struct packet_stack_t second_stack = { 
    //     .pre_build_response = 0, .post_build_response = 0,
    //     .packet_pointers = tcp_3_way_handshake_3, .write_chain_length = 1 
    // };

    // second_stack.packet_pointers[1] = tcp_3_way_handshake_3 + 20;

    // if(handler->operations.read(&second_stack, config->interface, handler)) {
    //     return false;
    // }

    // // check that no response was send

    // struct packet_stack_t third_stack = { 
    //     .pre_build_response = 0, .post_build_response = 0,
    //     .packet_pointers = tcp_3_way_handshake_4, .write_chain_length = 1 
    // };

    // third_stack.packet_pointers[1] = tcp_3_way_handshake_4 + 20;

    // if(handler->operations.read(&third_stack, config->interface, handler)) {
    //     return false;
    // }

    // check that the correct reponse was send and that it is equal to tcp_3_way_handshake_5

    return true;
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
        .operations.write = tcp_test_write,
        .ipv4_addr = 0x42bafa8e,
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

    struct test_run_t tcp_test_run = {
        .config = &test_config,
        .handler = tcp_handler,
        .tests = tests       
    };

    run_tests(&tcp_test_run);
}



