#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "handlers/handler.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define IP_HEADER_SIZE 14
#define ETHERNET_HEADER_SIZE 18


struct test_config_t {
    struct interface_t* interface;
    
};

struct test_t {
    char name[45];
    bool (*test)(struct handler_t* handler, struct test_config_t* config);   
};

struct test_run_t {
    struct handler_t* handler;
    struct test_config_t* config;

    struct test_t** tests;
};

void* test_malloc(const char *type, size_t size);

bool run_tests(struct test_run_t* test_run);

#endif // TEST_COMMON_H