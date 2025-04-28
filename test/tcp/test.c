#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/socket.h"
#include "handlers/tcp/tcp_block_buffer.h"
#include "handlers/ethernet/ethernet.h"

#include "test/tcp/test.h"
#include "test/common.h"
#include "test/tcp/overrides.h"
#include "test/tcp/tests/download_1/download_1.h"


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>

void before_each(struct test_run_t* test_run) {
    // create stub tcp handler
    struct handler_config_t* handler_config = malloc(sizeof(struct handler_config_t));
    handler_config->mem_allocate = test_malloc;
    handler_config->mem_free = free;
    handler_config->write = 0; // must be set by individual tests

    struct tcp_priv_config_t priv_config = {
        .window = 4098
    };

    struct handler_t* tcp_handler = tcp_create_handler(handler_config); 
	tcp_handler->init(tcp_handler, &priv_config);

    test_run->handler = tcp_handler;
}

void after_each(struct test_run_t* test_run) {
    // should probably destroy handler here 
}

/*************************************
 *          BOOTSTRAP                *
**************************************/
bool tcp_tests_start() {  
    // setup test interface configurations      
    struct interface_t interface = {
        .port = 0,
        .operations.write = 0, // must be set by individual tests 
        .ipv4_addr = 0, // must be set by individual tests
        .mac = 0
    };
    
    struct test_config_t test_config = { 
        .interface = &interface
    };	


    // Initialize tests
    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(test->name, "3-Way Handshake, data transfer, and reset", sizeof("3-Way Handshake, data transfer, and reset"));    
    test->test = tcp_test_download_1;

    // queue tests
    struct test_t* tests[2];
    tests[0] = test;
    tests[1] = 0; // zero out last pointer to mark end of array

    // run tests
    struct test_run_t tcp_test_run = {
        .config = &test_config,
        .handler = 0, // is set by the before_each function
        .tests = tests,
        .before_each = before_each,
        .after_each = after_each
    };

    run_tests(&tcp_test_run);
}



