#include "socket.h"
#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "tcp_shared.h"
#include "tcp_block_buffer.h"
#include "log.h"

#include <string.h>
#include <arpa/inet.h>

void tcp_delete_transmission_control_block(struct transmission_control_block_t* tcb, struct handler_t* handler) {
    tcp_block_buffer_destroy(tcb->in_buffer, handler->handler_config->mem_free);
    handler->handler_config->mem_free(tcb->out_buffer);
    handler->handler_config->mem_free(tcb);
}



// struct socket* create_socket(uint32_t connection_id, struct handler_t* handler) {
    
//     struct transmission_control_block_t* tcb = 
//         (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", 
//             sizeof(struct transmission_control_block_t));

//     tcb->id = connection_id;    

//     tcb->own_ipv4 = ipv4_request->destination_ip;
//     tcb->remote_ipv4 = ipv4_request->source_ip;
    
//     tcb->send_initial_sequence_num = tcp_shared_generate_sequence_number();
//     tcb->send_next = tcb->send_initial_sequence_num;
//     tcb->send_window = 512; // hard-coded for now 

//     tcb->send_unacknowledged = 0;
//     tcb->send_urgent_pointer = 0;
//     tcb->send_last_update_sequence_num = 0;
//     tcb->send_last_update_acknowledgement_num = 0;        
    
//     tcb->receive_initial_sequence_num = ntohl(tcp_request->sequence_num);
//     tcb->receive_window = ((struct tcp_priv_t*) handler->priv)->receive_window;
//     tcb->receive_urgent_pointer = tcp_request->urgent_pointer;
//     tcb->receive_next = tcb->receive_initial_sequence_num;

//     tcb->in_buffer = create_tcp_block_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);
//     tcb->out_buffer = (struct tcp_header_t*) handler->handler_config->mem_allocate("tcp(tcb) output buffer", sizeof(struct tcp_header_t));

//     tcb->state = start_state; 
    
//     tcb->state_function = state_function;

//     return tcb;
// }

struct transmission_control_block_t* tcp_create_transmission_control_block(struct tcp_socket_t* socket, ...) {
    for (uint64_t i = 0; i < TCP_SOCKET_NUM_TCB; i++) {
        if(!socket->trans_control_block[i]) {
            socket->trans_control_block[i] = tcb;
            return true;
        }        
    }
    return false;;          
}

/**
 * Get the transmission control block associated with the given connection id on the given socket.
 */
struct transmission_control_block_t* tcp_get_transmission_control_block(struct tcp_socket_t* socket, uint32_t connection_id) {
    for (uint8_t i = 0 ; i < TCP_SOCKET_NUM_TCB ; i++) {
        struct transmission_control_block_t* tcb = socket->trans_control_block[i];
        
        if(tcb && tcb->id == connection_id) {
            return tcb;
        } 
    }   
    return 0; 
}

/**
 * Get an existing tcp socket. Sockets should be stored in the handler's private storage
 */
struct tcp_socket_t* tcp_get_socket(struct handler_t* handler, uint32_t ipv4, uint16_t port) {
    struct tcp_priv_t* priv = (struct tcp_priv_t*) handler->priv;

    for (uint8_t i = 0 ; i < SOCKET_BUFFER_SIZE && priv->tcp_sockets[i] ; i++) {
        struct tcp_socket_t* socket = priv->tcp_sockets[i];
        
        if(socket->listening_port == port && socket->ipv4 == ipv4) {
            return socket;
        } 
    }   
    return 0; 
}

/**
 * Deletes socket by freeing memory of all its structures
 */
void tcp_delete_socket(struct handler_t* handler, struct tcp_socket_t* socket) {
    for (uint8_t i = 0 ; i < TCP_SOCKET_NUM_TCB ; i++) {
        struct transmission_control_block_t* tcb = socket->trans_control_block[i];
        
        if(tcb) {
            tcp_delete_transmission_control_block(handler, tcb);
        } 
    }   
}

/**
 * Deletes the given transmission control block from socket by freeing memory of all its structures 
 * and disassociating it from the socket
 */
void tcp_delete_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t connection_id) {
    for (uint8_t i = 0 ; i < TCP_SOCKET_NUM_TCB ; i++) {
        struct transmission_control_block_t* tcb = socket->trans_control_block[i];
        
        if(tcb && tcb->id == connection_id) {
            tcp_delete_transmission_control_block(handler, tcb);
            socket->trans_control_block[i] = 0;
        } 
    }   
}

/**
 * Resets the socket by putting it into a default state which all sockets have on initial initilization
 */
void tcp_reset_socket(struct tcp_socket_t* socket) {
    memset(socket->trans_control_block, 0, sizeof(socket->trans_control_block));
}

/**
 * Called by other libraries to open a tcp connection on a specific port 
 */
uint8_t tcp_add_socket(struct handler_t* handler, struct tcp_socket_t* socket) {
    struct tcp_priv_t* priv = (struct tcp_priv_t*) handler->priv;

    for (uint64_t i = 0; i < SOCKET_BUFFER_SIZE; i++) {
        if(!priv->tcp_sockets[i]) {
            priv->tcp_sockets[i] = socket;
            return true;
        }        
    }
    return false;     
}

