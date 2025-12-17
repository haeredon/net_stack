#include "handlers/tls/handshake/finished.h"

#include <string.h>


uint32_t tls_write_finished(void* verify_data, uint16_t length, void* to_write) {
    memcpy(to_write, verify_data, length);

    return length;
}