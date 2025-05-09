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

bool is_tcp_packet_equal(struct tcp_header_t* a, struct tcp_header_t* b, struct tcp_cmp_ignores_t* ignfores);

void set_state_from_test_header(struct tcp_header_t* header);

#endif // TEST_TCP_UTILITY_H