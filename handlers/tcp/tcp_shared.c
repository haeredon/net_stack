
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>


uint32_t tcp_shared_generate_sequence_number() {
    return 42;
}

uint32_t tcp_shared_calculate_connection_id(uint32_t remote_ip, uint16_t remote_port, uint16_t host_port) {
    uint64_t id = ((uint64_t) remote_ip) << 32;
    id |= ((uint32_t) remote_port) << 16;
    id |= host_port;
    return id;
}

uint16_t tcp_calculate_checksum(struct tcp_pseudo_header_t* pseudo_header, struct tcp_header_t* header)  {
    uint32_t sum = 0;
    uint16_t length = ntohs(pseudo_header->tcp_length) / 2;

    uint16_t* ip = (uint16_t*) pseudo_header;
    uint16_t* tcp = (uint16_t*) header;

    for(uint16_t i = 0 ; i < 6 ; ++i) {
        sum += ip[i];
    }

    for(uint16_t i = 0 ; i < length ; ++i) {
        sum += tcp[i];
    }

    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (uint16_t) ~sum;
}

uint16_t _tcp_calculate_checksum(struct tcp_header_t* tcp_header, uint32_t source_ip, uint32_t destination_ip)  {
        struct tcp_pseudo_header_t pseudo_header = {
            .source_ip = source_ip,
            .destination_ip = destination_ip,
            .zero = 0,
            .ptcl = 6,
            .tcp_length = 0
        };

        return tcp_calculate_checksum(&pseudo_header, tcp_header);
}    

uint16_t tcp_get_payload_length(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    // it is multiplied by 4 because the header sizes are calculated in 32 bit chunks
    return ntohs(ipv4_header->total_length) - 
        (((tcp_header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) + (ipv4_header->flags_1 & IPV4_IHL_MASK)) * 4;
}

void* tcp_get_payload(struct tcp_header_t* tcp_header) {
    // it is multiplied by 4 because the header sizes are calculated in 32 bit chunks
    return ((uint8_t*) tcp_header) + ((tcp_header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;
}

