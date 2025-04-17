#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/tcp_tcb.h"
#include "handlers/tcp/tcp_block_buffer.h"
#include "handlers/ethernet/ethernet.h"

#include "test.h"
#include "test/common.h"
#include "overrides.h"
#include "tests/download_1/download_1.h"


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>


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

    // create stub tcp handler
    struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free,
        .write = 0 // must be set by individual tests
    };	

    struct handler_t* tcp_handler = tcp_create_handler(&handler_config); 
	tcp_handler->init(tcp_handler);

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
        .handler = tcp_handler,
        .tests = tests       
    };

    run_tests(&tcp_test_run);
}



