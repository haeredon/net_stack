#include "socket.h"
#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_shared.h"
#include "tcp_block_buffer.h"
#include "log.h"

#include <string.h>
#include <arpa/inet.h>

// void tcp_tcb_destroy_transmission_control_block(struct transmission_control_block_t* tcb, struct handler_t* handler) {
//     tcp_block_buffer_destroy(tcb->in_buffer, handler->handler_config->mem_free);
//     handler->handler_config->mem_free(tcb->out_buffer);
//     handler->handler_config->mem_free(tcb);
// }

void tcp_tcb_reset_transmission_control_blocks() {
    memset(transmission_blocks, 0, sizeof(transmission_blocks));
}

struct socket* create_socket(uint32_t connection_id, struct handler_t* handler) {
    
    struct transmission_control_block_t* tcb = 
        (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", 
            sizeof(struct transmission_control_block_t));

    tcb->id = connection_id;    

    tcb->own_ipv4 = ipv4_request->destination_ip;
    tcb->remote_ipv4 = ipv4_request->source_ip;
    
    tcb->send_initial_sequence_num = tcp_shared_generate_sequence_number();
    tcb->send_next = tcb->send_initial_sequence_num;
    tcb->send_window = 512; // hard-coded for now 

    tcb->send_unacknowledged = 0;
    tcb->send_urgent_pointer = 0;
    tcb->send_last_update_sequence_num = 0;
    tcb->send_last_update_acknowledgement_num = 0;        
    
    tcb->receive_initial_sequence_num = ntohl(tcp_request->sequence_num);
    tcb->receive_window = ((struct tcp_priv_t*) handler->priv)->receive_window;
    tcb->receive_urgent_pointer = tcp_request->urgent_pointer;
    tcb->receive_next = tcb->receive_initial_sequence_num;

    tcb->in_buffer = create_tcp_block_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);
    tcb->out_buffer = (struct tcp_header_t*) handler->handler_config->mem_allocate("tcp(tcb) output buffer", sizeof(struct tcp_header_t));

    tcb->state = start_state; 
    
    tcb->state_function = state_function;

    return tcb;
}