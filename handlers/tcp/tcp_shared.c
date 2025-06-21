#include "tcp_shared.h"

#include <stdint.h>

uint32_t tcp_shared_generate_sequence_number() {
    return 42;
}

uint32_t tcp_shared_calculate_connection_id(uint32_t source_ip, uint16_t source_port, uint16_t destination_port) {
    uint64_t id = ((uint64_t) source_ip) << 32;
    id |= ((uint32_t) source_port) << 16;
    id |= destination_port;
    return id;
}
