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



const uint8_t OWN_MAC[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
const uint8_t REMOTE_MAC[] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
const uint8_t BROADCAST_MAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const uint8_t OWN_IP[] = { 0x14, 0x14, 0x14, 0x14 };
const uint8_t REMOTE_IP[] = { 0x0A, 0x0A, 0x0A, 0x0A };



#define MAC_SIZE 6
#define FAIL 1
#define SUCCESS 0

struct response_t {
    void* response_buffer;
    uint64_t size;
    struct response_t* next;
};

struct responses_head_t {
    struct response_t* next;
    struct response_t* last;
} responses;

int64_t write_response(void* buffer, uint64_t size) {
    printf("Write called\n");

    struct response_t* new_response = (struct response_t*) malloc(sizeof(struct response_t*));
    new_response->response_buffer = buffer;
    new_response->size = size;
    
    struct response_t* response = responses.last;    
    
    if(response) {
        response->next = new_response;
    } else {
        responses.next = new_response;
        responses.last = new_response;
    }
}

struct interface_t interface = {
    .port = 0,
    .queue = 0,
    .operations = {
        .write = write_response
    }
};

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
        int num_bytes_read = reader->read_block(reader, req_buffer, MAX_BUFFER_SIZE);
        if(num_bytes_read == -1)  {
            printf("FAIL.\n");	
            return -1;
        }

        if(packet_is_type(req_buffer, PCAPNG_ENHANCED_BLOCK)) {
            printf("Enhanced Block!\n");    
            
            struct packet_enchanced_block_t* pcapng_block = (struct packet_enchanced_block_t*) req_buffer;

            // get packet ftom req_buffer
            struct ethernet_header_t* header = (struct ethernet_header_t*) &pcapng_block->packet_data;
            struct packet_stack_t packet_stack = { .response = 0, .packet_pointers = header, .write_chain_length = 0 };


            // TODO: support broadcast
            
            uint8_t destionation_is_remote = !memcmp(header->destination, REMOTE_MAC, 6);
            uint8_t source_is_remote = !memcmp(header->source, REMOTE_MAC, 6);

            uint8_t destionation_is_own = !memcmp(header->destination, OWN_MAC, 6);
            uint8_t source_is_own = !memcmp(header->source, OWN_MAC, 6);

            // if ethernet destination is the remote mac and source is own mac 
            // then sent it to be read by the handler
            if(destionation_is_own && source_is_remote) {
                test->handler->operations.read(&packet_stack, &interface, test->handler->priv);
            } else if(destionation_is_remote && source_is_own) {
                struct response_t* response = responses.next;

                if(response) {
                    uint8_t is_same_size = response->size == pcapng_block->captured_package_length;
                    
                    if(is_same_size &&
                       memcmp(response->response_buffer, &pcapng_block->packet_data, response->size)
                    ) { 
                        // fix response structure
                        if(responses.next == responses.last) {
                            responses.last = 0;
                        }
                        responses.next = responses.next->next;
                        free(response);
                    } else {
                        printf("FAIL!\n");
                        return FAIL;   
                    }
                } else {
                    printf("FAIL!\n");
                    return FAIL;    
                }
            } else {
                printf("FAIL!\n");
                return FAIL;
            }

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
    struct test_t* test_suite = create_test_suite();

    if(test_suite->init(test_suite, handlers[1])) { // we know ethernet is index 1. That needs to be made more stable in the future
        return -1;
    }

    test_suite->test(test_suite);
    test_suite->end(test_suite);
}



