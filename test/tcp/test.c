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
#include "test/tcp/tests/active_mode/active_mode.h"


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

    struct tcp_priv_config_t tcp_priv_config = {
        .window = 4098
    };
  
    struct handler_t* tcp_handler = tcp_create_handler(handler_config); 
	tcp_handler->init(tcp_handler, &tcp_priv_config);

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
    struct test_t* download_1_test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(download_1_test->name, "3-Way Handshake, data transfer, and FIN", sizeof("3-Way Handshake, data transfer, and FIN"));    
    download_1_test->test = tcp_test_download_1;

    // struct test_t* active_mode_test = (struct test_t*) malloc(sizeof(struct test_t));
    // strncpy(active_mode_test->name, "Active mode: Handshake and send data followed by FIN", sizeof("Active mode: Handshake and send data followed by FIN"));    
    // active_mode_test->test = tcp_test_active_mode;

    // queue tests
    struct test_t* tests[2];
    tests[0] = download_1_test;
    // tests[1] = active_mode_test;
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



