#ifndef HANDLERS_TCP_TCB_H
#define HANDLERS_TCP_TCB_H

#include <stdint.h>
#include <stdbool.h>


#include "handlers/ipv4/ipv4.h"
#include "tcp_shared.h"

#define TCP_RECEIVE_WINDOW 4098

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

    uint16_t (*state_function)(
        struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
};


void tcp_tcb_destroy_transmission_control_block(struct transmission_control_block_t* tcb, struct handler_t* handler);

struct transmission_control_block_t* create_transmission_control_block(uint32_t connection_id, 
    struct tcp_socket_t* socket, struct tcp_header_t* tcp_request, 
    struct ipv4_header_t* ipv4_request, enum TCP_STATE start_state, void* state_function, 
    struct handler_t* handler);

struct transmission_control_block_t* get_transmission_control_block(uint32_t id);

bool tcp_add_transmission_control_block(struct transmission_control_block_t* tcb);

extern struct transmission_control_block_t* transmission_blocks[];

#endif // HANDLERS_TCP_TCB_H