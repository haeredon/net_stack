#include "test/common.h"

#include <stdio.h>
#include <stdbool.h>

void* test_malloc(const char *type, size_t size) {
    return malloc(size);
}


bool run_tests(struct test_run_t* test_run) {

    struct test_t** tests = test_run->tests;

    while(*tests) {
        struct test_t* test = (*tests);

        printf("Testing: %s\n", test->name);

        test_run->before_each(test_run);
        bool success = test->test(test_run->handler, test_run->config);
        test_run->after_each(test_run);
    
        if(success) {
            printf("\tSUCCESS\n");
        } else {
            printf("\tFAIL\n");
        }      
        
        tests++;  
    }
}