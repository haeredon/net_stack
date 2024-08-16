#include "handlers/ipv4.h"
#include "handlers/null.h"
#include "handlers/ethernet.h"
#include "test.h"
#include "test/common.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Ethernet II, Src: bb:bb:bb:bb:bb:bb (bb:bb:bb:bb:bb:bb), Dst: aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa)
// Internet Protocol Version 4, Src: 192.168.0.117, Dst: 32.32.32.32
//     0100 .... = Version: 4
//     .... 0101 = Header Length: 20 bytes (5)
//     Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
//     Total Length: 20
//     Identification: 0x92b8 (37560)
//     010. .... = Flags: 0x2, Don't fragment
//     ...0 0000 0000 0000 = Fragment Offset: 0
//     Time to Live: 64
//     Protocol: TCP (6)
//     Header Checksum: 0xa6ce [correct]
//     [Header checksum status: Good]
//     [Calculated Checksum: 0xa6ce]
//     Source Address: 192.168.0.117
//     Destination Address: 32.32.32.32
static const unsigned char ipv4_1[34] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, /* ........ */
    0xbb, 0xbb, 0xbb, 0xbb, 0x08, 0x00, 0x45, 0x00, /* ......E. */
    0x00, 0x14, 0x92, 0xb8, 0x40, 0x00, 0x40, 0x06, /* ....@.@. */
    0xa6, 0xce, 0xc0, 0xa8, 0x00, 0x75, 0x20, 0x20, /* .....u   */
    0x20, 0x20                                      /*    */
};



int64_t ipv4_test_write(struct response_t response) {
        
}

bool ipv4_test(struct handler_t* handler, struct test_config_t* config) {
    struct packet_stack_t packet_stack = { 
        .pre_build_response = 0, .post_build_response = 0,
        .packet_pointers = &ipv4_1, .write_chain_length = 0 };
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    return true;
}


bool ipv4_tests_start() {        
    struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free 
    };	

    struct interface_t interface = {
        .port = 0,
        .port = 0,
        .operations.write = 0,
        .ipv4_addr = OWN_IP,
        .mac = 0
    };
    memcpy(&interface, OWN_MAC, ETHERNET_MAC_SIZE);

    struct test_config_t test_config = { 
        .interface = &interface
    };	

    // add an extra null handler to handle unimplemented protocols
    struct handler_t* null_handler = null_create_handler(&handler_config); // TODO: fix memory leak
	null_handler->init(null_handler);

    ADD_TO_PRIORITY(&ip_type_to_handler, 0x01, null_handler); // ICMP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, null_handler); // TCP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x11, null_handler); // UDP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x29, null_handler); // IPv6

	struct handler_t* ipv4_handler = ipv4_create_handler(&handler_config); 
	ipv4_handler->init(ipv4_handler);

    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(test->name, "Basic Test", sizeof("Basic Test"));    
    test->test = ipv4_test;

    struct test_t* tests[2];
    tests[0] = test;
    tests[1] = 0; // zero out last pointer

    struct test_run_t ipv4_test_run = {
        .config = &test_config,
        .handler = ipv4_handler,
        .tests = tests       
    };

    run_tests(&ipv4_test_run);
}



