#ifndef HANDLER_ID_H
#define HANDLER_ID_H

#include "handler.h"
#include "handlers/handler.h"

struct id_priv_t {
    int dummy;
};

#define ID_MAGIC_NUMBER 0xFADBADFADBADDEAD

struct id_header_t {
    uint64_t magic_number;
    uint64_t num_bytes_before;
};


struct id_header_t* id_get_id_header(const uint8_t* data, const uint64_t size) {
    const uint16_t minimum_size = sizeof(struct id_header_t);

    if(size < minimum_size) {
        return 0;
    }

    struct id_header_t* id_header = (struct id_header_t*) ((data + size) - sizeof(struct id_header_t));

    if(id_header->magic_number == ID_MAGIC_NUMBER) {
        return id_header;
    }

    return 0;
}


struct handler_t* id_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_ID_H