#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handler.h"


struct tcp_priv_t {
    int dummy;
};

#define TCP_SYN_FLAG 2


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

struct transmission_control_block_t {
    uint32_t id;

    uint32_t remote_ipv4;
    uint32_t own_ipv4;

    uint16_t remote_port;
    uint16_t own_port;

    uint32_t send_unacknowledged;
    uint32_t send_next;
    uint16_t send_window;
    uint16_t send_urgent_pointer;
    uint32_t send_initial_sequence_num;
    uint32_t send_last_update_sequence_num;
    uint32_t send_last_update_acknowledgement_num;

    uint32_t receive_next;
    uint16_t receive_window;
    uint16_t receive_urgent_pointer;
    uint32_t receive_initial_sequence_num;

    enum TCP_STATE state;
};

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

struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);





#endif // HANDLERS_TCP_H