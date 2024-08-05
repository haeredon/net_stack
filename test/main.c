#include "test_suite.h"
 
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


void* test_malloc(const char *type, size_t size) {
    return malloc(size);
}

int main(int argc, char **argv) {        
    struct handler_config_t handler_config = { 
        .mem_allocate = test_malloc, 
        .mem_free = free 
    };	
    struct handler_t** handlers = handler_create_stacks(&handler_config);        
    
    // start testing
    struct test_suite_t* test_suite = test_suite_create_test_suite("arp.pcapng");

    if(test_suite->init(test_suite, handlers[1])) { // we know ethernet is index 1. That needs to be made more stable in the future
        return -1;
    }

    test_suite->test(test_suite);
    test_suite->end(test_suite);
}



