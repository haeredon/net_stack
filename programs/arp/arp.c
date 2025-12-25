#include <stdio.h>

#include "handlers/socket.h"
#include "handlers/arp/socket.h"
#include "handlers/arp/arp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/ipv4/ipv4.h"
#include "programs/arp/arp.h"
#include "net_stack.h"

#include <arpa/inet.h>
#include <string.h>

void arp_program_start(const struct net_stack_app* net_stack) {
    struct arp_socket_t* socket = arp_create_socket(0, true);
    arp_set_socket(net_stack->arp_handler, socket);
}

void arp_spoof(const struct net_stack_app* net_stack, struct arp_socket_t* socket) {
    struct arp_write_args_t arp_write_args = {
        .header = {
            .hdw_type = ARP_HDW_TYPE_ETHERNET,
            .pro_type = ARP_PRO_TYPE_IPV4,
            .hdw_addr_length = ETHERNET_MAC_SIZE,
            .pro_addr_length = IPV4_ADDR_SIZE,
            .operation = ARP_OPERATION_REQUEST,
            .sender_hardware_addr = 0,
            .sender_protocol_addr = net_stack->interface->ipv4_addr,
            .target_hardware_addr = 0,
            .target_protocol_addr = net_stack->interface->ipv4_addr
        }
    };
    memcpy(arp_write_args.header.sender_hardware_addr, net_stack->interface->mac, ETHERNET_MAC_SIZE);

    struct ethernet_write_args_t ethernet_write_args;
    memcpy(&ethernet_write_args.destination, net_stack->interface->mac, ETHERNET_MAC_SIZE);            
    ethernet_write_args.ethernet_type = ETHERNET_TYPE_ARP;

   struct socket_client_args write_args = {
        .handlers = { net_stack->ethernet_handler, net_stack->arp_handler },        
        .handler_args = { &ethernet_write_args, &arp_write_args},
        .depth = 2
    };

    socket->operations.send(net_stack->arp_handler, socket, &write_args);        
}

void arp_status(const struct net_stack_app* net_stack, struct arp_socket_t* socket) {
    struct arp_status_t* status = socket->operations.status(net_stack->arp_handler, socket);

    printf("ARP Status: %d entries\n", status->num_arp_entries);
    for (uint16_t i = 0; i < status->num_arp_entries; i++) {
        struct arp_entry_t* entry = &status->entries[i];
        struct in_addr ipv4_addr;
        ipv4_addr.s_addr = htonl(entry->ipv4);
        printf("Entry %d: IP %s -> MAC %02x:%02x:%02x:%02x:%02x:%02x\n", i,
            inet_ntoa(ipv4_addr),
            entry->mac[0], entry->mac[1], entry->mac[2],
            entry->mac[3], entry->mac[4], entry->mac[5]);
    }

    NET_STACK_FREE(status);
}