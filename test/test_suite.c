#include <string.h>
#include <stdlib.h>

#include "test_suite.h"
#include "pcapng.h"
#include "handlers/handler.h"
#include "handlers/ethernet.h"
#include "handlers/id.h"

const uint8_t OWN_MAC[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
const uint8_t REMOTE_MAC[] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
const uint8_t BROADCAST_MAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

const uint32_t OWN_IP = 0x20202020;

#define FAIL 1
#define SUCCESS 0


int64_t test_suite_write_response(struct response_t response) {
    // trick to get the responses list from the test_suite
    uint64_t interface_offset = offsetof(struct test_suite_t, interface);
    uint64_t responses_offset = offsetof(struct test_suite_t, responses);
    struct test_responses_head_t* responses = 
        (struct test_responses_head_t*) (((uint8_t*) response.interface) - interface_offset + responses_offset);

    // add a new response to the reponse list
    struct test_response_t* new_response = (struct test_response_t*) malloc(sizeof(struct test_response_t*));
    new_response->response_buffer = response.buffer;
    new_response->size = response.size;
    new_response->uses_id_header = 0;
    new_response->next = 0;
    
    struct id_header_t* id_header = id_get_id_header(response.buffer, response.size);

    if(id_header) {    
        new_response->size = id_header->num_bytes_before;
        new_response->uses_id_header = 1;
    }
    
    struct test_response_t* last_response = responses->last;    
    
    if(last_response) {
        last_response->next = new_response;
        responses->last = new_response;
    } else {
        responses->next = new_response;
        responses->last = new_response;
    }
}


uint8_t test_suite_test(struct test_suite_t* test_suite) {
    static const int MAX_BUFFER_SIZE = 0xFFFF;
    uint8_t* req_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    uint8_t* res_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    struct pcapng_reader_t* reader = test_suite->reader;

    struct test_responses_head_t* responses = &test_suite->responses;

    while(reader->has_more_headers(reader)) {
        int num_bytes_read = reader->read_block(reader, req_buffer, MAX_BUFFER_SIZE);
        if(num_bytes_read == -1)  {
            return FAIL;
        }

        if(packet_is_type(req_buffer, PCAPNG_ENHANCED_BLOCK)) {
            
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
                test_suite->handler->operations.read(&packet_stack, &test_suite->interface, test_suite->handler->priv);
            } else if(destionation_is_remote && source_is_own) {
                struct test_response_t* response = responses->next;

                if(response) {                                        
                    uint8_t is_same_size = response->size == pcapng_block->captured_package_length;
                    
                    if((is_same_size || response->uses_id_header) &&
                       !memcmp(response->response_buffer, &pcapng_block->packet_data, response->size)
                    ) { 
                        // fix response structure
                        if(responses->next == responses->last) {
                            responses->last = 0;
                        }
                        responses->next = responses->next->next;
                        free(response);
                    } else {
                        return FAIL;   
                    }
                } else {
                    return FAIL;    
                }
            } else {
                return FAIL;
            }
        }
    }
}

int test_suite_init(struct test_suite_t* test_suite, struct handler_t* handler) {
    test_suite->reader = create_pcapng_reader();        

    if(test_suite->reader->init(test_suite->reader, test_suite->pcapng_test_file)) {
        test_suite->reader->close(test_suite->reader);
        return -1;
    };    
    
    test_suite->handler = handler;
    test_suite->handler->init(test_suite->handler);

    return 0;
}

void test_suite_end(struct test_suite_t* test_suite) {
    test_suite->reader->close(test_suite->reader);
    test_suite->handler->close(test_suite->handler);
    free(test_suite->pcapng_test_file);
    free(test_suite);
}

struct test_suite_t* test_suite_create_test_suite(char pcapng_test_file[]) {
    struct test_suite_t* test_suite = (struct test_suite_t*) malloc(sizeof(struct test_suite_t));

    test_suite->init = test_suite_init;
    test_suite->end = test_suite_end;
    test_suite->test = test_suite_test;  

    uint64_t file_name_size = strlen(pcapng_test_file) + 1; // plus null byte;
    test_suite->pcapng_test_file = (char*) malloc(file_name_size);
    strcpy(test_suite->pcapng_test_file, pcapng_test_file);

    test_suite->responses.last = 0;
    test_suite->responses.next = 0;

    test_suite->interface.port = 0;
    test_suite->interface.queue = 0;
    test_suite->interface.operations.write = test_suite_write_response;
    test_suite->interface.ipv4_addr = OWN_IP;
    memcpy(test_suite->interface.mac, OWN_MAC, ETHERNET_MAC_SIZE);

    return test_suite;
}
