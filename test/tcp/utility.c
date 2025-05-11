#include <stdbool.h>

#include "test/tcp/utility.h"
#include "handlers/tcp/tcp.h"

#include <string.h>


uint16_t get_tcp_header_length(struct tcp_header_t* header) {
    return ((header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;    
}

void* get_tcp_payload(struct tcp_header_t* header) {
    return ((uint8_t*) header) + get_tcp_header_length(header);
}

bool is_tcp_package_equal(struct tcp_header_t* a, struct tcp_header_t* expected, uint64_t packet_size, struct tcp_cmp_ignores_t* ignoares) {
    bool headers_equal = a->source_port == expected->source_port &&
    a->destination_port == expected->destination_port &&
    (a->sequence_num == expected->sequence_num || ignoares->sequence_num) &&
    a->acknowledgement_num == expected->acknowledgement_num &&
    (a->data_offset == expected->data_offset || ignoares->data_offset) &&
    a->control_bits == expected->control_bits &&
    a->window == expected->window &&
    (a->checksum == expected->checksum || ignoares->checksum) &&
    a->urgent_pointer == expected->urgent_pointer;

    void* actual_payload = get_tcp_payload(a);
    void* expected_payload = get_tcp_payload(expected);

    uint16_t actual_header_length = get_tcp_header_length(a);
    uint16_t expected_header_length = get_tcp_header_length(expected);

    return headers_equal && 
        actual_header_length == expected_header_length &&
        !memcmp(actual_payload, expected, packet_size - actual_header_length);
}

struct tcp_cmp_ignores_t ignores = {
    .sequence_num = false,
    .checksum = true,
    .data_offset = true
};

