#ifndef HANDLER_ETHERNET_H
#define HANDLER_ETHERNET_H

#include "worker.h"
#include "handlers/handler.h"

#define ETHERNET_NUM_ETH_TYPE_ENTRIES 512

struct ethernet_priv_t {
    int dummy;
};

struct ethernet_header_t
{
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t ethernet_type; // should have different name in the future to better supppoprt vlan tagging
};


extern struct key_val_t ethernet_type_to_handler[ETHERNET_NUM_ETH_TYPE_ENTRIES];


void ethernet_close_handler(struct handler_t* handler);

void ethernet_init_handler(struct handler_t* handler);

uint16_t ethernet_read(struct rte_mbuf* buffer, struct interface_t* interface, void* priv);

struct handler_t* ethernet_create_handler();



#endif // HANDLER_ETHERNET_H