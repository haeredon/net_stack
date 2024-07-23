#ifndef HANDLER_ARP_H
#define HANDLER_ARP_H

#include "handler.h"
#include "handlers/handler.h"
#include "handlers/ethernet.h"

#define ETHERNET_NUM_ETH_TYPE_ENTRIES 512

struct arp_priv_t {
    int dummy;
};

struct arp_header_t {
    uint16_t hdw_type;
    uint16_t pro_type;
    uint8_t hdw_addr_length;
    uint8_t pro_addr_length;
    uint16_t operation;
    uint8_t sender_hardware_addr[ETHERNET_MAC_SIZE];
    uint32_t sender_protocol_addr[4];
    uint8_t target_hardware_addr[ETHERNET_MAC_SIZE];
    uint32_t target_protocol_addr[4];
};


struct handler_t* arp_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_ARP_H