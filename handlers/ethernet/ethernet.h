#ifndef HANDLER_ETHERNET_H
#define HANDLER_ETHERNET_H

#include "handlers/handler.h"
#include "handlers/protocol_map.h"

#define ETHERNET_NUM_ETH_TYPE_ENTRIES 512

#define ETHERNET_MAC_SIZE 6
#define ETHERNET_TAG_SIZE 6
#define ETHERNET_ETH_TYPE_SIZE 6

// big endian types
#define ETHERNET_TYPE_ARP 0x0806
#define ETHERNET_TYPE_IPV6 0x8
#define ETHERNET_TYPE_IPV4 0x0800

struct ethernet_priv_t {
    int dummy;
};

struct ethernet_write_args_t {
    uint8_t destination[6];
    uint16_t ethernet_type;
};

struct ethernet_header_t{
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t ethernet_type; // should have different name in the future to better supppoprt vlan tagging
};


extern struct priority_map_t ethernet_type_to_handler;

void ethernet_init_header(void* buffer, uint8_t* source, uint8_t* dest, uint8_t* tag, uint8_t* type);

struct handler_t* ethernet_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_ETHERNET_H