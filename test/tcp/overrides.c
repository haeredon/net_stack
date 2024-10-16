#include "handlers/tcp/tcp_shared.h"
#include "overrides.h"

uint32_t current_sequence_number = 0;

uint32_t tcp_shared_generate_sequence_number() {
    return current_sequence_number;
}
