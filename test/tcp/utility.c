#include <stdbool.h>

#include "test/tcp/utility.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/ipv4/ipv4.h"

#include <arpa/inet.h>
#include <string.h>


bool is_tcp_header_equal(struct tcp_header_t* a, struct tcp_header_t* expected, struct tcp_cmp_ignores_t* ignores) {
    return a->source_port == expected->source_port &&
    a->destination_port == expected->destination_port &&
    (a->sequence_num == expected->sequence_num || ignores->sequence_num) &&
    a->acknowledgement_num == expected->acknowledgement_num &&
    (a->data_offset == expected->data_offset || ignores->data_offset) &&
    a->control_bits == expected->control_bits &&
    (a->window == expected->window || ignores->window) &&
    (a->checksum == expected->checksum || ignores->checksum) &&
    a->urgent_pointer == expected->urgent_pointer;
}

struct tcp_cmp_ignores_t ignores = {
    .sequence_num = false,
    .checksum = true,
    .data_offset = true,
    .window = true
};

