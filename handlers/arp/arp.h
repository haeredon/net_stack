#ifndef HANDLER_ARP_H
#define HANDLER_ARP_H

#include "handlers/handler.h"
#include "handlers/ethernet/ethernet.h"

#define ETHERNET_NUM_ETH_TYPE_ENTRIES 512

#define ARP_OPERATION_REQUEST 256 // big_endian
#define ARP_OPERATION_RESPOENSE 512 // big_endian

#define ARP_HDW_TYPE_ETHERNET 256 // big_endian

struct arp_header_t {
    uint16_t hdw_type;
    uint16_t pro_type;
    uint8_t hdw_addr_length;
    uint8_t pro_addr_length;
    uint16_t operation;
    uint8_t sender_hardware_addr[ETHERNET_MAC_SIZE];
    uint32_t sender_protocol_addr;
    uint8_t target_hardware_addr[ETHERNET_MAC_SIZE];
    uint32_t target_protocol_addr;
} __attribute__((packed));

struct arp_priv_t {
    int dummy;
};

struct arp_write_args_t {
    struct arp_header_t header;    
};

struct arp_entry_t {
    uint32_t ipv4;
    uint8_t mac[ETHERNET_MAC_SIZE];
};

struct arp_resoltion_list_t {
    struct arp_entry_t** list;
    uint16_t insert_idx;
    pthread_rwlock_t lock;
};

struct arp_entry_t* arp_get_ip_mapping(uint32_t ipv4);
struct handler_t* arp_create_handler(struct handler_config_t *handler_config);



#endif // HANDLER_ARP_H