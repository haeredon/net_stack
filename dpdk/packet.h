#ifndef DPDK_PACKET_H
#define DPDK_PACKET_H

#include "interface.h"

struct interface_t** interfaces;

void* dpdk_packet_get_packet_buffer(void* packet);
void dpdk_packet_free_packet(void* packet);
struct interface_t* dpdk_packet_get_interface(void* packet);

#endif // DPDK_PACKET_H