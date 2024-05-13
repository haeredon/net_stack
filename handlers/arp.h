#ifndef HANDLER_ARP_H
#define HANDLER_ARP_H

#include "worker.h"
#include "handlers/handler.h"

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
};

struct ipv4_arp_t {
    uint8_t sender_hardware_addr[6];
    uint32_t sender_protocol_addr[4];
    uint8_t target_hardware_addr[6];
    uint32_t target_protocol_addr[4];
};

void arp_close_handler(struct handler_t* handler);

void arp_init_handler(struct handler_t* handler);

uint16_t arp_read(struct rte_mbuf* buffer, struct interface_t* interface, void* priv);

struct handler_t* arp_create_handler();



#endif // HANDLER_ARP_H