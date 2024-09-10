#ifndef HANDLER_NULL_H
#define HANDLER_NULL_H

#include "handlers/handler.h"

struct id_priv_t {
    int dummy;
};

#define ID_MAGIC_NUMBER 0xFADBADFADBADDEAD

struct null_header_t {
    uint64_t magic_number;
    uint64_t num_bytes_before;
};


struct null_header_t* null_get_id_header(const uint8_t* data, const uint64_t size);


struct handler_t* null_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_NULL_H