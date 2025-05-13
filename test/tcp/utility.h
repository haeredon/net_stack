#ifndef TEST_TCP_UTILITY_H
#define TEST_TCP_UTILITY_H

#include "handlers/tcp/tcp_shared.h"
#include "test/tcp/overrides.h"

#include <stdbool.h>

struct tcp_cmp_ignores_t {
    bool sequence_num;
    bool checksum; 
    bool data_offset;
};

extern struct tcp_cmp_ignores_t ignores;

bool is_tcp_header_equal(struct tcp_header_t* a, struct tcp_header_t* b, struct tcp_cmp_ignores_t* ignores);

/**
 * Package Management
 */
uint16_t get_tcp_header_length(struct tcp_header_t* header);

void* get_tcp_payload(struct tcp_header_t* header);

struct ipv4_header_t* get_ipv4_header_from_package(const void* package);

struct tcp_header_t* get_tcp_header_from_package(const void* package);
uint16_t get_tcp_payload_length_from_package(const void* package);

void* get_tcp_payload_payload_from_package(const void* package); 

#endif // TEST_TCP_UTILITY_H