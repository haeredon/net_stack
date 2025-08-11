
#ifndef HANDLERS_TCP_TCP_STATES_H
#define HANDLERS_TCP_TCP_STATES_H

#include "handlers/tcp/socket.h"
#include "handlers/tcp/tcp_shared.h"

#include <stdbool.h>


uint16_t tcp_close(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
uint16_t tcp_syn_sent(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);

bool is_acknowledgement_valid(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header);
bool is_segment_in_window(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header, uint32_t payload_size);

#endif // HANDLERS_TCP_TCP_STATES_H