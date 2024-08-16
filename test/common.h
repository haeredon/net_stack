#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "handlers/handler.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

extern const uint8_t OWN_MAC[];
extern const uint8_t REMOTE_MAC[];
extern const uint8_t BROADCAST_MAC[];
extern const uint32_t OWN_IP;

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