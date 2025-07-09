
#ifndef HANDLERS_TCP_TCP_STATES_H
#define HANDLERS_TCP_TCP_STATES_H




uint16_t tcp_close(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);
uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface):


#endif // HANDLERS_TCP_TCP_STATES_H