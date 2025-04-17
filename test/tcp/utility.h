#ifndef TEST_TCP_UTILITY_H
#define TEST_TCP_UTILITY_H

#include "handlers/tcp/tcp_shared.h"
#include "overrides.h"

#include <stdbool.h>

struct tcp_cmp_ignores_t {
    bool sequence_num;
    bool checksum; 
    bool data_offset;
};

struct tcp_cmp_ignores_t ignores = {
    .sequence_num = false,
    .checksum = true,
    .data_offset = true
};

bool is_tcp_packet_equal(struct tcp_header_t* a, struct tcp_header_t* b, struct tcp_cmp_ignores_t* ignores);

void set_state_from_test_header(struct tcp_header_t* header);

#endif // TEST_TCP_UTILITY_H