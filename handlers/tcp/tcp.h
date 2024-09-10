#ifndef HANDLERS_TCP_H
#define HANDLERS_TCP_H

#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_out_buffer.h"
#include "tcp_block_buffer.h"

#include <stdbool.h>

struct tcp_priv_t {
    int dummy;
};

#define TCP_SYN_FLAG 2
#define TCP_ACK_FLAG 16

#define TCP_DATA_OFFSET_MASK 0xF0

#define TCP_RECEIVE_WINDOW 4098
#define TCP_OUT_BUFFER_SIZE 4098

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

struct tcp_socket_t {
    uint16_t listening_port;
    struct interface_t* interface;
};

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

struct transmission_control_block_t {
    uint32_t id;

    struct tcp_socket_t* socket;

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

    struct tcp_block_buffer_t* in_buffer;
    struct tcp_out_buffer_t* out_buffer;

    uint16_t (*state_function)(
        struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header, 
        struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb);
};


bool tcp_add_socket(struct tcp_socket_t* socket, struct handler_t* handler);


struct handler_t* tcp_create_handler(struct handler_config_t *handler_config);

#endif // HANDLERS_TCP_H