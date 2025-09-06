#include "test/arp/request_packages.h"
#include "test/arp/test.h"
#include "test/common.h"
#include "test/utility.h"
#include "util/array.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/arp/arp.h"


#include <string.h>

/*************************************
 *          UTILITY                  *
**************************************/

static struct out_buffer_t* arp_response_buffer;

static bool nothing(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    arp_response_buffer = &packet_stack->out_buffer;
    return true;
}

bool is_arp_header_equal(struct arp_header_t* a, struct arp_header_t* expected) {
    return !memcmp(a, expected, sizeof(struct arp_header_t));
}

/*************************************
 *          TESTS                *
**************************************/

bool arp_test_basic(struct handler_t* handler, struct test_config_t* config) {
    struct ethernet_header_t* ethernet_first_header = get_ethernet_header_from_package(pkt1);
    struct arp_header_t* arp_first_header = get_arp_header_from_package(pkt1);

    struct handler_config_t handler_config = {
        .write = 0 
    };

    struct handler_t* ethernet_handler = ethernet_create_handler(&handler_config);
    ethernet_handler->operations.write = nothing;
     
    config->interface->ipv4_addr = arp_first_header->target_protocol_addr;
    memcpy(config->interface->mac, ethernet_first_header->destination, ETHERNET_MAC_SIZE);

    /*************** TEST 1 (Probe to correct interface) ******************/
    struct in_packet_stack_t packet_stack = { 
        .stack_idx = 1, 
        .handlers = ethernet_handler 
    };

    packet_stack.in_buffer.packet_pointers[0] = get_ethernet_header_from_package(pkt1); // we need this extra layer because arp depends on the underlying protocol
    packet_stack.in_buffer.packet_pointers[1] = get_arp_header_from_package(pkt1);
    packet_stack.in_buffer.packet_pointers[2] = 0;

    // probe when target is the interface
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    struct arp_header_t* expected_arp_header = get_arp_header_from_package(pkt2);
    struct arp_header_t* actual_arp_header = (struct arp_header_t*) ((uint8_t*) arp_response_buffer->buffer + arp_response_buffer->offset);
    
    if(!is_arp_header_equal(expected_arp_header, actual_arp_header)) {
        return false;
    }

    /*************** TEST 2 (Probe to correct interface, udpate of existing mac to ip mapping) ******************/

    // send new sender mac address
    unsigned char packet[sizeof(pkt1)];
    memcpy(packet, pkt1, sizeof(pkt1));

    struct arp_header_t* arp_header = get_arp_header_from_package(packet);
    arp_header->sender_protocol_addr = 1;
    memset(&arp_header->sender_hardware_addr, 1, ETHERNET_MAC_SIZE);
    packet_stack.in_buffer.packet_pointers[1] = arp_header;

    // probe when target is the interface
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }

    actual_arp_header = (struct arp_header_t*) ((uint8_t*) arp_response_buffer->buffer + arp_response_buffer->offset);
    struct arp_entry_t* arp_entry = arp_get_ip_mapping(arp_header->sender_protocol_addr);
    
    if(!memcmp(arp_entry->mac, actual_arp_header->sender_hardware_addr, ETHERNET_MAC_SIZE) && 
        arp_entry->ipv4 == actual_arp_header->sender_protocol_addr) {
        return false;
    }

    /***************  TEST 3 (Probe to non correct interface) ******************/
    config->interface->ipv4_addr = 42; // have bogus interface address
    
     // no reponse for this one, so we empty the buffer and check if it stays empty
    memset(arp_response_buffer->buffer, 0 , arp_response_buffer->size);
    
    // probe when target is NOT the interface
    if(handler->operations.read(&packet_stack, config->interface, handler)) {
        return false;
    }
    
    if(!array_is_zero(arp_response_buffer->buffer, arp_response_buffer->size)) {
        return false;
    }    

    return true;
}


/*************************************
 *          BOOTSTRAP                *
**************************************/
bool arp_tests_start() {
  struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free 
    };	

    struct interface_t interface = {
        .port = 0,
        .port = 0,
        .operations.write = 0,
        .ipv4_addr = 0,  // must be set by individual tests
        .mac = 0  // must be set by individual tests
    };

    struct test_config_t test_config = { 
        .interface = &interface
    };	

 	struct handler_t* arp_handler = arp_create_handler(&handler_config); 
	arp_handler->init(arp_handler, 0);

    struct test_t* probe_test = (struct test_t*) malloc(sizeof(struct test_t));
    strncpy(probe_test->name, "ARP Probe", sizeof("ARP Probe"));    
    probe_test->test = arp_test_basic;

    struct test_t* tests[2];
    tests[0] = probe_test;
    tests[1] = 0; // zero out last pointer to mark end of array

    struct test_run_t arp_test_run = {
        .config = &test_config,
        .handler = arp_handler,
        .tests = tests,
        .before_each = 0,
        .after_each = 0
    };

    run_tests(&arp_test_run);
}