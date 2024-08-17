#include "common.h"

#include <stdio.h>

const uint8_t OWN_MAC[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
const uint8_t REMOTE_MAC[] = { 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB };
const uint8_t BROADCAST_MAC[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const uint32_t OWN_IP = 0x20202020;

void* test_malloc(const char *type, size_t size) {
    return malloc(size);
}


bool run_tests(struct test_run_t* test_run) {

    struct test_t** tests = test_run->tests;

    while(*tests) {
        struct test_t* test = (*tests);

        printf("Testing: %s\n", test->name);

        bool success = test->test(test_run->handler, test_run->config);
    
        if(success) {
            printf("\tSUCCESS\n");
        } else {
            printf("\tFAIL\n");
        }      
        
        tests++;  
    }
}