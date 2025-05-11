#ifndef CUSTOM_NULL_H
#define CUSTOM_NULL_H

#include "handlers/handler.h"

struct custom_priv_t {
    void* response_buffer;
    uint32_t response_length;
};

struct custom_header_t {
    uint64_t magic_number;
    uint64_t num_bytes_before;
};

struct handler_t* custom_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_NULL_H