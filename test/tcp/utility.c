#include <stdbool.h>

#include "test/tcp/utility.h"



bool is_tcp_packet_equal(struct tcp_header_t* a, struct tcp_header_t* b, struct tcp_cmp_ignores_t* ignoares) {
    return a->source_port == b->source_port &&
    a->destination_port == b->destination_port &&
    (a->sequence_num == b->sequence_num || ignoares->sequence_num) &&
    a->acknowledgement_num == b->acknowledgement_num &&
    (a->data_offset == b->data_offset || ignoares->data_offset) &&
    a->control_bits == b->control_bits &&
    a->window == b->window &&
    (a->checksum == b->checksum || ignoares->checksum) &&
    a->urgent_pointer == b->urgent_pointer;
}

void set_state_from_test_header(struct tcp_header_t* header) {
    current_sequence_number = 2189314807;
}


