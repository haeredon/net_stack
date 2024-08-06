#ifndef HANDLER_IPV4_H
#define HANDLER_IPV4_H

#include "handler.h"
#include "handlers/handler.h"
#include "handlers/ethernet.h"


struct ipv4_priv_t {
    int dummy;
};

struct ipv4_header_t {
    uint8_t flags_1;
    uint8_t flags_2;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_3;
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t destination_ip;
    // no support for options!
} __attribute__((packed));


struct handler_t* ipv4_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_IPV4_H