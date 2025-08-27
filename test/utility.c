#include "test/utility.h"

#include <arpa/inet.h>

uint16_t get_tcp_header_length(struct tcp_header_t* header) {
    return ((header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;    
}

void* get_tcp_payload(struct tcp_header_t* header) {
    return ((uint8_t*) header) + get_tcp_header_length(header);
}

struct ipv4_header_t* get_ipv4_header_from_package(const void* package) {    
    return (struct ipv4_header_t*) (((uint8_t*) package) + sizeof(struct ethernet_header_t));
}

struct tcp_header_t* get_tcp_header_from_package(const void* package) {
    struct ipv4_header_t* ipv4 = get_ipv4_header_from_package(package);
    uint16_t ipv4_header_length = (ipv4->flags_1 & IPV4_IHL_MASK) * sizeof(uint32_t);

    return (struct tcp_header_t*)(((uint8_t*) ipv4) + ipv4_header_length);
}

struct arp_header_t* get_arp_header_from_package(const void* package) {
    return (struct arp_header_t*) (((uint8_t*) package) + sizeof(struct ethernet_header_t));
}

struct ethernet_header_t* get_ethernet_header_from_package(const void* package) {
    return (struct ethernet_header_t*) package;
}

uint16_t get_tcp_payload_length_from_package(const void* package) {
    struct ipv4_header_t* ipv4 = get_ipv4_header_from_package(package);    
    uint16_t ipv4_header_length = (ipv4->flags_1 & IPV4_IHL_MASK) * sizeof(uint32_t);
    uint16_t ipv4_package_length = ntohs(ipv4->total_length);

    struct tcp_header_t* tcp_header = get_tcp_header_from_package(package);
    uint16_t tcp_header_length = get_tcp_header_length(tcp_header);

    return ipv4_package_length - ipv4_header_length - tcp_header_length;
}

void* get_tcp_payload_payload_from_package(const void* package) {
    struct tcp_header_t* tcp_header = get_tcp_header_from_package(package);
    uint16_t tcp_header_length = get_tcp_header_length(tcp_header);

    return ((uint8_t*) tcp_header) + tcp_header_length;
}