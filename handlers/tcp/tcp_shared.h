#ifndef HANDLERS_TCP_SHARED_H
#define HANDLERS_TCP_SHARED_H

#include <stdint.h>

#define TCP_SYN_FLAG 2
#define TCP_ACK_FLAG 16
#define TCP_RST_FLAG 4
#define TCP_FIN_FLAG 1

struct tcp_pseudo_header_t {
    uint32_t source_ip;
    uint32_t destination_ip;
    uint8_t zero;
    uint8_t ptcl;
    uint16_t tcp_length;
} __attribute__((packed, aligned(2)));

struct tcp_header_t {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence_num;
    uint32_t acknowledgement_num;
    uint8_t data_offset;
    uint8_t control_bits;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_pointer;    
} __attribute__((packed, aligned(2)));

enum TCP_STATE {
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK,
    TIME_WAIT
};

uint32_t tcp_shared_generate_sequence_number();

#endif // HANDLERS_TCP_SHARED_H