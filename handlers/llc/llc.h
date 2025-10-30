#ifndef HANDLER_HANDLERS_LLC_H
#define HANDLER_HANDLERS_LLC_H

#include "handlers/handler.h"


#define LLC_NUM_PROTOCOL_TYPE_ENTRIES 64

#define LLC_UNUMBERED_FORMAT 0x11


struct llc_priv_t {
    int dummy;
};

struct llc_write_args_t {
    uint8_t destination_access_service_point;
    uint8_t source_access_service_point;
    uint8_t control; // can be 16 bit, but we do not support it in the implementation
};

struct llc_header_t {
    uint8_t destination_access_service_point;
    uint8_t source_access_service_point;
    uint8_t control; // can be 16 bit, but we do not support it in the implementation    
} __attribute__((packed, aligned(2)));

struct handler_t* llc_create_handler(struct handler_config_t *handler_config);

extern struct priority_map_t llc_type_to_handler;



#endif // HANDLER_HANDLERS_LLC_H