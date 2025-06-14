#ifndef HANDLER_IPV4_H
#define HANDLER_IPV4_H

#include "handlers/handler.h"
#include "handlers/ethernet/ethernet.h"


#define IP_NUM_PROTOCOL_TYPE_ENTRIES 64

#define IPV4_IHL_MASK 0x0F

struct ipv4_priv_t {
    int dummy;
};

struct ipv4_write_args_t {
    uint32_t destination_ip; // network representation
    uint8_t protocol;
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
} __attribute__((packed, aligned(2)));


extern struct priority_map_t ip_type_to_handler;

struct handler_t* ipv4_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_IPV4_H