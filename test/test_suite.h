#ifndef TEST_TEST_SUITE_H
#define TEST_TEST_SUITE_H

#include <stdint.h>

#include "handlers/handler.h"

struct test_response_t {
    void* response_buffer;
    uint64_t size;
    uint8_t uses_id_header;
    struct test_response_t* next;
};

struct test_responses_head_t {
    struct test_response_t* next;
    struct test_response_t* last;
};

struct test_suite_t {
    uint8_t (*test)(struct test_suite_t* test);
    int (*init)(struct test_suite_t* test, struct handler_t* handler);
    void (*end)(struct test_suite_t* test);

    struct test_responses_head_t responses;
    struct interface_t interface;

    char* pcapng_test_file;

    int64_t (*write_response)(void* buffer, uint64_t size);

    struct pcapng_reader_t* reader;
    struct handler_t* handler;
};

struct test_suite_t* test_suite_create_test_suite(char pcapng_test_file[]);


#endif // _TEST_SUITE_H