#include "handlers/tcp/tcp.h"
#include "handlers/tcp/tcp_states.h"
#include "handlers/ipv4/ipv4.h"
#include "util/log.h"
#include "tcp_block_buffer.h"
#include "tcp_shared.h"
#include "socket.h"
#include "util/memory.h"

#include <string.h>
#include <arpa/inet.h>

/*
 * Transmission Control Protocol.
 * Specification: RFC 9293
 * 
 * Currently only supports ipv4
 * 
 * No support for 
 *  - urgent pointer
 *  - options 
 *  - checksum validation
 *  - piggybacking ACKs on segments which should be sent anyway
 *  - PUSH. Everything will be considered as push
 * 
 * Buffer overflow dangers:     
 *      1. Response buffer size
 *      2. Number of connections
 *      3. Number of listeners
 * 
 * Known issues
 *  - Only check and push output queue for a transmission control block when it actually has activity from a client
 *  - Doen't flush when to server/client when receiving FIN
 *  - Does not handle URG bit
 */


//////////////////////////////// NOTES //////////////////////////////////////
// ESTABLISHED STATE 
// CLOSE-WAIT STATE
//     - different in segment text. Should just not occur
//     - different in fin check. just remain in close-wait

// FIN-WAIT-1 STATE 
// FIN-WAIT-2 STATE
//     - different in ack check. add 1 additional check
//     - different in fin check. enter TIME-WAIT



void tcp_close_handler(struct handler_t* handler) {
    struct tcp_priv_t* private = (struct tcp_priv_t*) handler->priv;    
    NET_STACK_FREE(private);
}

void tcp_init_handler(struct handler_t* handler, void* priv_config) {    
    struct tcp_priv_t* tcp_priv = (struct tcp_priv_t*) NET_STACK_MALLOC("tcp handler private data", sizeof(struct tcp_priv_t)); 

    struct tcp_priv_config_t* config = (struct tcp_priv_config_t*) priv_config;

    if(!config) {
        NETSTACK_LOG(NETSTACK_ERROR, "No config defined for tcp handler.\n");     
    }

    tcp_priv->window = config->window;

    int init_lock_res = pthread_mutex_init(&tcp_priv->socket_list_lock, 0);
    if(init_lock_res) {
        NETSTACK_LOG(NETSTACK_ERROR, "TCP: Failed to initialize private storage lock for handler\n");   
    }

    handler->priv = (void*) tcp_priv;
}

/*
* This function assumes that destination id always is the same
*/
uint32_t tcb_header_to_connection_id(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    return tcp_shared_calculate_connection_id(ipv4_header->source_ip, tcp_header->source_port, tcp_header->destination_port);    
}


bool tcp_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct tcp_write_args_t* tcp_args = (struct tcp_write_args_t*) packet_stack->args[packet_stack->stack_idx];

    if(!packet_stack->stack_idx) {
        NETSTACK_LOG(NETSTACK_WARNING, "TCP as the bottom header in packet stack is not possible, yet it happended.\n");   
        return false;
    }

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    struct tcp_header_t* response_header = (struct tcp_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct tcp_header_t) -4); // we don't calculate in options here

    uint64_t payload_size = out_buffer->size - out_buffer->offset;

    if(out_buffer->offset < sizeof(struct tcp_header_t)) { // we don't calculate in options here
        NETSTACK_LOG(NETSTACK_WARNING, "No room for tcp header in buffer\n");         
        return false;   
    };

    if(!tcp_args->socket) {
        NETSTACK_LOG(NETSTACK_WARNING, "Could not find socket.\n");   
        return false;
    }

    if(!tcp_args->connection_id) {
        NETSTACK_LOG(NETSTACK_WARNING, "Could not find connection id.\n");   
        return false;
    }

    struct transmission_control_block_t* tcb = tcp_get_transmission_control_block(tcp_args->socket, tcp_args->connection_id);

    if(!tcb) {
        NETSTACK_LOG(NETSTACK_WARNING, "Could not find transmission control block.\n");   
        return false;
    }

    struct tcp_header_t* out_header = (struct tcp_header_t*) tcb->out_header;
    out_header->source_port = tcp_args->socket->port;
    out_header->destination_port = tcb->remote_port;
    out_header->data_offset = (sizeof(struct tcp_header_t) / 4 + 1) << 4; // data offset is counted in 32 bit chunks 
    out_header->window = htons(tcb->receive_window);
    out_header->urgent_pointer = 0; // Not supported
    out_header->control_bits = tcp_args->flags; 

    out_header->sequence_num = htonl(tcb->send_next);
    out_header->acknowledgement_num = htonl(tcb->receive_next);

    uint32_t kage = 0xb4050402;
    memcpy(tcb->out_header + 20, &kage, 4);

    if(payload_size) {
        out_header->control_bits |= TCP_PSH_FLAG;
        tcb->send_next += payload_size;        
    }

    out_header->checksum = _tcp_calculate_checksum(out_header, tcp_args->socket->ipv4, tcb->remote_ipv4);
    
    memcpy(response_header, tcb->out_header, sizeof(struct tcp_header_t) + 4); 
    out_buffer->offset -= sizeof(struct tcp_header_t) + 4;
 
    // add buffer to outgoing block buffer
    tcp_block_buffer_add(tcb->out_buffer, packet_stack, out_header->sequence_num, payload_size);
        
    // then iterate over all outgoin buffers 
    struct tcp_block_t* buffer_block;
    while(buffer_block = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->out_buffer)) {
        struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) buffer_block->data;
    
        out_buffer = &out_package_stack->out_buffer;
        response_header = (struct tcp_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct tcp_header_t) - 4); // we don't calculate in options here
        
        uint16_t remote_window = ntohs(tcb->send_window);
        payload_size = buffer_block->payload_size;        

        // if the outgoing package fit in the send window then send it, else just abort but keep the buffer for later
        if(remote_window >= payload_size) {
            // this can't be the bottom if the stack because TCP is dependent on IP, hence no check for the bottom
            const struct handler_t* next_handler = out_package_stack->handlers[--packet_stack->stack_idx];
   
            if(!next_handler->operations.write(out_package_stack, interface, next_handler)) {
                return false;
            }

            /** SHOULD PROBABLY UPDATE THE SEND WINDOW SOMEWHERE AROUND HERE!!!!! */

            tcp_block_buffer_remove_front(tcb->out_buffer, 1);         
        } else {
            break;
        }        
    }

    return true;    
}

uint16_t tcp_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) { 
    packet_stack->handlers[packet_stack->stack_idx] = handler;  
    struct tcp_header_t* header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
    
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];
    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, header);
    
    struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, header->destination_port);

    if(socket) {
        struct transmission_control_block_t* tcb = tcp_get_transmission_control_block(socket, connection_id);

        if(!tcb) {
            if(header->control_bits & TCP_SYN_FLAG) {
                tcb = tcp_create_transmission_control_block(handler, socket, connection_id, 
                header, ipv4_header->source_ip, LISTEN);
                tcp_add_transmission_control_block(socket, tcb);
            
                if(!tcb) {
                    NET_STACK_FREE(tcb);
                    NETSTACK_LOG(NETSTACK_INFO, "Could not allocate transmission control block.\n");          
                    return 1;
                } 
            } else {
                NETSTACK_LOG(NETSTACK_INFO, "Received TCP initial header without SYN bits enabled.\n");          
                return 1;
            }
        } 

        uint16_t tcp_payload_size = tcp_get_payload_length(ipv4_header, header);
        tcp_block_buffer_add(tcb->in_buffer, packet_stack, header->sequence_num, tcp_payload_size);

        if(tcb->state == SYN_SENT) {
            tcb->receive_next = ntohl(header->sequence_num);
        }
        
        uint16_t num_ready = tcp_block_buffer_num_ready(tcb->in_buffer, tcb->receive_next);
        
        if(num_ready) {
            return tcb->state_function(handler, tcb, num_ready, interface); 
        }
    }

    return 0;
}

struct handler_t* tcp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) NET_STACK_MALLOC("tcp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = tcp_init_handler;
    handler->close = tcp_close_handler;

    handler->operations.read = tcp_read;
    handler->operations.write = tcp_write;        

    ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, handler); 

    return handler;
}