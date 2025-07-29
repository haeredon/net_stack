#ifndef HANDLERS_TCP_SHARED_H
#define HANDLERS_TCP_SHARED_H

#include "handlers/ipv4/ipv4.h"

#include <stdint.h>

#define TCP_SYN_FLAG 2
#define TCP_ACK_FLAG 16
#define TCP_RST_FLAG 4
#define TCP_FIN_FLAG 1
#define TCP_PSH_FLAG 8

#define SOCKET_BUFFER_SIZE 64
#define TCP_WINDOW 4096
#define TCP_HEADER_MAX_SIZE 60

struct tcp_priv_config_t {
    int window;
};

struct tcp_priv_t {
    int window;
    struct tcp_socket_t* tcp_sockets[SOCKET_BUFFER_SIZE];

    pthread_mutex_t socket_list_lock;
};

struct tcp_output_buffer_t {
    struct response_buffer_t* buffer;
    struct packet_stack_t* packet_stack;
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

enum TCP_STATE {
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    CLOSED,
    LAST_ACK,
    TIME_WAIT
};

uint32_t tcp_shared_generate_sequence_number();

uint32_t tcp_shared_calculate_connection_id(uint32_t source_ip, uint16_t source_port, uint16_t destination_port);

uint16_t _tcp_calculate_checksum(struct tcp_header_t* tcp_header, uint32_t source_ip, uint32_t destination_ip);

uint16_t tcp_get_payload_length(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header);

void* tcp_get_payload(struct tcp_header_t* tcp_header);

#endif // HANDLERS_TCP_SHARED_H