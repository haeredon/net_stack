#ifndef HANDLERS_TCP_SOCKET_H
#define HANDLERS_TCP_SOCKET_H

#include <stdint.h>
#include <stdbool.h>

#include "handlers/ipv4/ipv4.h"
#include "tcp_shared.h"
#include "tcp_block_buffer.h"

#define TCP_SOCKET_NUM_TCB 32

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
    uint32_t send_last_update_sequence_num; // called WL1 in RFC
    uint32_t send_last_update_acknowledgement_num; // called WL2 in RFC

    uint32_t receive_next;
    uint16_t receive_window; 
    uint16_t receive_urgent_pointer;
    uint32_t receive_initial_sequence_num;

    enum TCP_STATE state;

    struct tcp_block_buffer_t* in_buffer;
    struct tcp_header_t* out_buffer;

    uint16_t (*state_function)(struct handler_t* handler,
        struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
};

struct tcp_socket_t {
    uint16_t listening_port;
    uint32_t ipv4;
    struct transmission_control_block_t* trans_control_block[TCP_SOCKET_NUM_TCB];
};

struct transmission_control_block_t* tcp_create_transmission_control_block(struct tcp_socket_t* socket, ...);

struct transmission_control_block_t* tcp_get_transmission_control_block(struct tcp_socket_t* socket, uint32_t connection_id);

void tcp_delete_socket(struct handler_t* handler, struct tcp_socket_t* socket) ;

void tcp_delete_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t connection_id);

struct transmission_control_block_t* tcp_create_transmission_control_block(struct tcp_socket_t* socket, ...);

uint8_t tcp_add_socket(struct handler_t* handler, struct tcp_socket_t* socket);

struct tcp_socket_t* tcp_get_socket(struct handler_t* handler, uint32_t ipv4, uint16_t port);

#endif // HANDLERS_TCP_SOCKET_H