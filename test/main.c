#include "handlers/handler.h"
#include "handlers/null/null.h"
#include "handlers/protocol_map.h"
#include "handlers/ipv4/ipv4.h"
#include "ipv4/test.h"
#include "tcp/test.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>


#include "handlers/tcp/tcp_shared.h"

int main(int argc, char **argv) {     
    printf("Starting tests\n\n");     
    
    // ipv4_tests_start();
    tcp_tests_start();


    // struct handler_config_t handler_config = { 
    //     .mem_allocate = test_malloc, 
    //     .mem_free = free 
    // };	
    // struct handler_t** handlers = handler_create_stacks(&handler_config);        

    // // add an extra id handler to handle unimplemented protocols
    // struct handler_t* null_handler = null_create_handler(&handler_config); // TODO: fix memory leak
	// null_handler->init(null_handler);

    // ADD_TO_PRIORITY(&ip_type_to_handler, 0x01, null_handler); // ICMP
    // ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, null_handler); // TCP
    // ADD_TO_PRIORITY(&ip_type_to_handler, 0x11, null_handler); // UDP
    // ADD_TO_PRIORITY(&ip_type_to_handler, 0x29, null_handler); // IPv6
    
    // // start testing
    // struct test_suite_t* test_suites[] = {
    //     test_suite_create_test_suite("arp.pcapng"),
    //     test_suite_create_test_suite("ipv4.pcapng")
    // };

    // uint16_t num_test_suites = sizeof(test_suites) / sizeof(test_suites[0]);


    // for(uint16_t i = 0 ; i < num_test_suites ; i++) {        
    //     struct test_suite_t* test_suite = test_suites[i];
        
    //     printf("Testing: %s\n", test_suite->pcapng_test_file);

    //     if(test_suite->init(test_suite, handlers[1])) { // we know ethernet is index 1. That needs to be made more stable in the future
    //         return -1;
    //     }        
                
    //     test_suite->test(test_suite) ? printf("FAILED\n", test_suite->pcapng_test_file) :
    //         printf("SUCCESS\n", test_suite->pcapng_test_file);        

    //     test_suite->end(test_suite);
    // }
}



