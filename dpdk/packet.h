#ifndef DPDK_PACKET_H
#define DPDK_PACKET_H

void* dpdk_packet_get_packet_buffer(void* packet);
void dpdk_packet_free_packet(void* packet);

#endif // DPDK_PACKET_H