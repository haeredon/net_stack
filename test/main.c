#include "pcapng.h"
#include "handlers/handler.h"
#include "handlers/arp.h"
 
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


struct test_t {
    uint8_t (*test)(struct test_t* test);
    int (*init)(struct test_t* test, struct handler_t* handler);
    void (*end)(struct test_t* test);

    struct pcapng_reader_t* reader;
    struct handler_t* handler;
};


uint8_t test_test(struct test_t* test) {
    static const int MAX_BUFFER_SIZE = 0xFFFF;
    uint8_t* req_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    uint8_t* res_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    struct pcapng_reader_t* reader = test->reader;

    while(reader->has_more_headers(reader)) {
        if(reader->read_block(reader, req_buffer, MAX_BUFFER_SIZE) == -1)  {
            printf("FAIL.\n");	
            return -1;
        }

        if(packet_is_type(req_buffer, PCAPNG_ENHANCED_BLOCK)) {
            printf("Enhanced Block!\n");    

            // if(req_buffer is a response) {
            //   then evaluate against previous pending response
            // } else {
            //    if(there is already a previous reqponse in pending) {
            //        fail test;
            //        return:
            //    }
            //    struct packet_enchanced_block_t* block = (struct packet_enchanced_block_t*) req_buffer;
            //
            //    test->handler->operations.read(&block->packet_data, 0, 0, test->handler->priv);            
            // }            

            printf("Do test!\n");
        }
    }
}

int test_init(struct test_t* test, struct handler_t* handler) {
    test->reader = create_pcapng_reader();        

    if(test->reader->init(test->reader, "arp.pcapng")) {
        test->reader->close(test->reader);
        return -1;
    };    
    
    test->handler = handler;
    test->handler->init(test->handler);

    return 0;
}

void test_end(struct test_t* test) {
    test->reader->close(test->reader);
    test->handler->close(test->handler);
}

struct test_t* create_test_suite() {
    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));

    test->init = test_init;
    test->end = test_end;
    test->test = test_test;    
}

/**********************************************/

void* test_malloc(const char *type, size_t size, unsigned align) {
    return malloc(size);
}

int main(int argc, char **argv) {    
    struct handler_t** handlers = handler_create_stacks(test_malloc);        
    
    // start testing
    struct test_t* test_suite = create_test_suite();

    if(test_suite->init(test_suite, handlers[1])) { // we know ethernet is index 1. That needs to be made more stable in the future
        return -1;
    }

    test_suite->test(test_suite);
    test_suite->end(test_suite);
}




// AA:AA:AA:AA:AA:AA is own mac
// BB:BB:BB:BB:BB:BB is remote mac
// 20.20.20.20 is own ip
// 10.10.10.10 is remote ip