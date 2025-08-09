#include "handlers/ipv4/ipv4.h"
#include "handlers/custom/custom.h"
#include "handlers/ethernet/ethernet.h"
#include "test/ipv4/test.h"
#include "test/common.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*************************************
 *          TEST DATA                *
**************************************/

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
static const unsigned char ipv4_1_req[20] = {
    0x45, 0x00, 0x00, 0x14, 0x92, 0xb8, 0x40, 0x00,
    0x40, 0x06, 0xa6, 0xce, 0xc0, 0xa8, 0x00, 0x75, 
    0x20, 0x20, 0x20, 0x20                                      
}; 

// Internet Protocol Version 4, Src: 32.32.32.32, Dst: 192.168.0.117
//     0100 .... = Version: 4
//     .... 0101 = Header Length: 20 bytes (5)
//     Differentiated Services Field: 0x00 (DSCP: CS0, ECN: Not-ECT)
//     Total Length: 20
//     Identification: 0x0000 (0)
//     010. .... = Flags: 0x2, Don't fragment
//     ...0 0000 0000 0000 = Fragment Offset: 0
//     Time to Live: 64
//     Protocol: TCP (6)
//     Header Checksum: 0x3987 [correct]
//     [Header checksum status: Good]
//     [Calculated Checksum: 0x3987]
//     Source Address: 32.32.32.32
//     Destination Address: 192.168.0.117
static const unsigned char ipv4_1_res[20] = {
    0x45, 0x00, 0x00, 0x14, 0x00, 0x00, 0x40, 0x00, 
    0x40, 0x06, 0x39, 0x87, 0x20, 0x20, 0x20, 0x20, 
    0xc0, 0xa8, 0x00, 0x75                                      
};



/*************************************
 *          UTILITY                  *
**************************************/

const uint8_t OWN_MAC[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
const uint8_t REMOTE_MAC[] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
const uint8_t BROADCAST_MAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const uint32_t OWN_IP = 0x20202020;

struct response_t last_response;
uint8_t response_buffer[4096];

int64_t ipv4_test_write(struct response_t response) {
    memcpy(&last_response, &response, sizeof(struct response_t));
    memcpy(&response_buffer, response.buffer, sizeof(response_buffer));
    last_response.buffer = response_buffer;
}

bool send_and_check_response(const uint8_t* request, const uint8_t* response, uint16_t response_size, struct handler_t* handler, struct test_config_t* config) {
    struct in_packet_stack_t packet_stack = { 
        .in_buffer = { .packet_pointers = realloc}, .stack_idx = 0 };
    
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    if(last_response.size != response_size ||
       memcmp(response_buffer, response, last_response.size)) {
        return false;
    }

    return true;
}

/*************************************
 *          TESTS                *
**************************************/
bool ipv4_test_basic(struct handler_t* handler, struct test_config_t* config) {
    return send_and_check_response(ipv4_1_req, ipv4_1_res, sizeof(ipv4_1_res), handler, config);
}

/*************************************
 *          BOOTSTRAP                *
**************************************/
bool ipv4_tests_start() {        
    struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free 
    };	

    struct interface_t interface = {
        .port = 0,
        .port = 0,
        .operations.write = ipv4_test_write,
        .ipv4_addr = OWN_IP,
        .mac = 0
    };
    memcpy(&interface, OWN_MAC, ETHERNET_MAC_SIZE);

    struct test_config_t test_config = { 
        .interface = &interface
    };	

    // add an extra null handler to handle unimplemented protocols
    struct handler_t* custom_handler = custom_create_handler(&handler_config); // TODO: fix memory leak
	custom_handler->init(custom_handler, 0);

    ADD_TO_PRIORITY(&ip_type_to_handler, 0x01, custom_handler); // ICMP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, custom_handler); // TCP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x11, custom_handler); // UDP
    ADD_TO_PRIORITY(&ip_type_to_handler, 0x29, custom_handler); // IPv6

	struct handler_t* ipv4_handler = ipv4_create_handler(&handler_config); 
	ipv4_handler->init(ipv4_handler, 0);

    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(test->name, "Basic Test", sizeof("Basic Test"));    
    test->test = ipv4_test_basic;

    struct test_t* tests[2];
    tests[0] = test;
    tests[1] = 0; // zero out last pointer to mark end of array

    struct test_run_t ipv4_test_run = {
        .config = &test_config,
        .handler = ipv4_handler,
        .tests = tests       
    };

    run_tests(&ipv4_test_run);
}



