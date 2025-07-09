#include "handlers/tcp/tcp.h"
#include "handlers/tcp/socket.h"
#include "handlers/handler.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp_shared.h"
#include "handlers/tcp/tcp_block_buffer.h"
#include "handlers/tcp/tcp_states.h"
#include "log.h"

#include <string.h>
#include <arpa/inet.h>

void delete_transmission_control_block(struct handler_t* handler, struct transmission_control_block_t* tcb) {
    tcp_block_buffer_destroy(tcb->in_buffer, handler->handler_config->mem_free);
    handler->handler_config->mem_free(tcb);
}

struct transmission_control_block_t* create_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, 
        uint32_t connection_id, const struct tcp_header_t* initial_header, 
        uint32_t source_ip, void* state_function) {
    struct tcp_priv_t* priv = (struct tcp_priv_t*) handler->priv;
    
    struct transmission_control_block_t* tcb = 
        (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", 
            sizeof(struct transmission_control_block_t));

    tcb->id = connection_id;    

    tcb->remote_ipv4 = source_ip;   

    tcb->remote_port = initial_header ? initial_header->source_port : 0;                           
    
    tcb->send_initial_sequence_num = tcp_shared_generate_sequence_number();
    tcb->send_next = tcb->send_initial_sequence_num;
    tcb->send_window = initial_header ? initial_header->window : 0;

    tcb->send_unacknowledged = 0;
    tcb->send_urgent_pointer = 0;
    tcb->send_last_update_sequence_num = 0;
    tcb->send_last_update_acknowledgement_num = 0;        
    
    tcb->receive_initial_sequence_num = initial_header ? ntohl(initial_header->sequence_num) : 0;       
    tcb->receive_window = priv->window;
    tcb->receive_urgent_pointer = initial_header ? initial_header->urgent_pointer : 0;                  
    tcb->receive_next = tcb->receive_initial_sequence_num;                      

    tcb->in_buffer = create_tcp_block_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);
    tcb->out_buffer = create_tcp_block_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);
    
    tcb->state_function = state_function;                              

    return tcb;
}

struct transmission_control_block_t* tcp_create_transmission_control_block(struct handler_t* handler, struct tcp_socket_t* socket, 
        uint32_t connection_id, const struct tcp_header_t* initial_header, 
        uint32_t source_ip, void* state_function) {
    for (uint64_t i = 0; i < TCP_SOCKET_NUM_TCB; i++) {
        if(!socket->trans_control_block[i]) {
            socket->trans_control_block[i] = create_transmission_control_block(handler, socket, 
                connection_id, initial_header, source_ip, state_function); 
            return socket->trans_control_block[i];
        }        
    }
    return 0;
}

/**
 * Get the transmission control block associated with the given connection id on the given socket.
 */
struct transmission_control_block_t* tcp_get_transmission_control_block(const struct tcp_socket_t* socket, uint32_t connection_id) {
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
struct tcp_socket_t* tcp_get_socket(const struct handler_t* handler, uint32_t ipv4, uint16_t port) {
    struct tcp_priv_t* priv = (struct tcp_priv_t*) handler->priv;

    for (uint8_t i = 0 ; i < SOCKET_BUFFER_SIZE && priv->tcp_sockets[i] ; i++) {
        struct tcp_socket_t* socket = priv->tcp_sockets[i];
        
        if(socket->port == port && socket->ipv4 == ipv4) {
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
            delete_transmission_control_block(handler, tcb);
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
            delete_transmission_control_block(handler, tcb);
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
bool tcp_add_socket(struct handler_t* handler, struct tcp_socket_t* socket) {
    struct tcp_priv_t* priv = (struct tcp_priv_t*) handler->priv;

    for (uint64_t i = 0; i < SOCKET_BUFFER_SIZE; i++) {
        if(!priv->tcp_sockets[i]) {
            priv->tcp_sockets[i] = socket;
            return true;
        }        
    }
    return false;     
}



// returns connection id of the tcb
uint32_t tcp_socket_open(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t remote_ip, uint16_t port) {

    // create connection id
    uint32_t connection_id = tcp_shared_calculate_connection_id(remote_ip, port, socket->ipv4);

    // stub tcp header for tcb initialization of active mode
    struct tcp_header_t initial_header = {
        .window = 0,
        .urgent_pointer = 0,
        .source_port = port,
        .window = 0
    };

    // create a TCB somehow        
    struct transmission_control_block_t* tcb = tcp_create_transmission_control_block(handler, socket, connection_id, 
                &initial_header, remote_ip, tcp_close);
            
    // initiate a handshake
    struct tcp_write_args_t tcp_args = {
        .connection_id = connection_id,
        .socket = socket,
        .flags = TCP_SYN_FLAG
    };

    struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) handler->handler_config->
                mem_allocate("response: tcp_package", DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct out_packet_stack_t)); 

    memcpy(out_package_stack->handlers, socket->socket.handlers, 10 * sizeof(struct handler_t*));
    memcpy(out_package_stack->args, socket->socket.handler_args, 10 * sizeof(void*));
    socket->socket.handler_args[socket->socket.depth - 1] = &tcp_args;

    out_package_stack->out_buffer.buffer = (uint8_t*) out_package_stack + sizeof(struct out_packet_stack_t);
    out_package_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_package_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;      

    out_package_stack->stack_idx = socket->socket.depth - 1;

    if(handler->operations.write(out_package_stack, socket->socket.interface, handler)) {
        NETSTACK_LOG(NETSTACK_ERROR, "TCP: Could not open socket connection\n");   
        return 1;
    } else {
        return connection_id;
    };
}

bool tcp_socket_send(struct tcp_socket_t* socket, uint32_t connection_id) {
    // just call the write function
}

void tcp_socket_close(struct tcp_socket_t* socket, uint32_t connection_id) {
    // send FINISH
    // potential clean up    
}

void tcp_socket_abort(struct tcp_socket_t* socket, uint32_t connection_id) {
    // send RESET
    // potential clean up    
}

void tcp_socket_status(struct tcp_socket_t* socket, uint32_t connection_id) {
    // return some status from TCB
}


struct tcp_socket_t tcp_create_socket(struct handler_t* next_handler, uint16_t port, uint32_t ipv4, 
    void (*receive)(uint8_t* data, uint64_t size)) {
        struct tcp_socket_t socket = {
            .ipv4 = ipv4,
            .port = port,
            .next_handler = next_handler, // deprecated
            .operations = {
                .open = tcp_socket_open,
                .send = tcp_socket_send,
                .receive = receive,
                .close = tcp_socket_close,
                .abort = tcp_socket_abort,
                .status = tcp_socket_status
            }
        };

        return socket;
}