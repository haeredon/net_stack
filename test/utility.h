#ifndef TEST_UTILITY_H
#define TEST_UTILITY_H

#include <stdint.h>

#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/arp/arp.h"

struct ipv4_header_t* get_ipv4_header_from_package(const void* package);
struct tcp_header_t* get_tcp_header_from_package(const void* package);
struct arp_header_t* get_arp_header_from_package(const void* package);
struct ethernet_header_t* get_ethernet_header_from_package(const void* package);

uint16_t get_tcp_payload_length_from_package(const void* package);

void* get_tcp_payload_payload_from_package(const void* package); 

#endif // TEST_UTILITY_H