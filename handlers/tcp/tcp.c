#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "log.h"
#include "tcp_block_buffer.h"
#include "tcp_shared.h"
#include "socket.h"

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
    handler->handler_config->mem_free(private);
}

void tcp_init_handler(struct handler_t* handler, void* priv_config) {    
    struct tcp_priv_t* tcp_priv = (struct tcp_priv_t*) handler->handler_config->mem_allocate("tcp handler private data", sizeof(struct tcp_priv_t)); 

    struct tcp_priv_config_t* config = (struct tcp_priv_config_t*) priv_config;

    if(!config) {
        NETSTACK_LOG(NETSTACK_ERROR, "No config defined for tcp handler.\n");     
    }

    tcp_priv->window = config->window;

    handler->priv = (void*) tcp_priv;
}

uint16_t tcp_calculate_checksum(struct tcp_pseudo_header_t* pseudo_header, struct tcp_header_t* header)  {
    uint32_t sum = 0;
    uint16_t length = ntohs(pseudo_header->tcp_length) / 2;

    uint16_t* ip = (uint16_t*) pseudo_header;
    uint16_t* tcp = (uint16_t*) header;

    for(uint16_t i = 0 ; i < 6 ; ++i) {
        sum += ip[i];
    }

    for(uint16_t i = 0 ; i < length ; ++i) {
        sum += tcp[i];
    }

    while (sum>>16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (uint16_t) ~sum;
}

uint16_t _tcp_calculate_checksum(struct tcp_header_t* tcp_header, uint32_t source_ip, uint32_t destination_ip)  {
        struct tcp_pseudo_header_t pseudo_header = {
            .source_ip = source_ip,
            .destination_ip = destination_ip,
            .zero = 0,
            .ptcl = 6,
            .tcp_length = 0
        };

        return tcp_calculate_checksum(&pseudo_header, tcp_header);
}    

uint16_t tcp_get_payload_length(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    // it is multiplied by 4 because the header sizes are calculated in 32 bit chunks
    return ntohs(ipv4_header->total_length) - 
        (((tcp_header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) + (ipv4_header->flags_1 & IPV4_IHL_MASK)) * 4;
}

void* tcp_get_payload(struct tcp_header_t* tcp_header) {
    // it is multiplied by 4 because the header sizes are calculated in 32 bit chunks
    return ((uint8_t*) tcp_header) + ((tcp_header->data_offset & TCP_DATA_OFFSET_MASK) >> 4) * 4;
}

bool is_acknowledgement_valid(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header) {
    uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);

    return tcb->send_unacknowledged < acknowledgement_num && 
       tcb->send_next >= acknowledgement_num;
}

bool is_segment_in_window(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header, uint32_t payload_size) {
    uint32_t sequence_num = ntohl(tcp_header->sequence_num);

    if(payload_size) {
        uint32_t sequence_end = sequence_num + payload_size - 1 /* -1 because it says in RFC, maybe think it through */;
        return tcb->receive_window && (sequence_num >= tcb->receive_next &&
               sequence_num < tcb->receive_next + tcb->receive_window && 
               sequence_end < tcb->receive_next + tcb->receive_window);
    } else {
        return ((tcb->receive_window) && (sequence_num >= tcb->receive_next &&
               sequence_num < tcb->receive_next + tcb->receive_window)) || 
               (!tcb->receive_window && tcb->receive_next == sequence_num);
    }
}

/*
* This function assumes that destination id always is the same
*/
uint32_t tcb_header_to_connection_id(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    return tcp_shared_calculate_connection_id(ipv4_header->source_ip, tcp_header->source_port, tcp_header->destination_port);    
}

bool tcp_internal_write(struct handler_t* handler, struct in_packet_stack_t* packet_stack, 
    struct tcp_socket_t* socket, uint32_t connection_id, uint8_t flags, uint8_t stack_idx, struct interface_t* interface) {
    
    struct tcp_write_args_t tcp_args = {
        .connection_id = connection_id,
        .socket = socket,
        .flags = flags
    };
    packet_stack->return_args[stack_idx] = &tcp_args;

    struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) handler->handler_config->
                mem_allocate("response: tcp_package", DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct out_packet_stack_t)); 

    memcpy(out_package_stack->handlers, packet_stack->handlers, 10 * sizeof(struct handler_t*));
    memcpy(out_package_stack->args, packet_stack->return_args, 10 * sizeof(void*));

    out_package_stack->out_buffer.buffer = (uint8_t*) out_package_stack + sizeof(struct out_packet_stack_t);
    out_package_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_package_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;      

    out_package_stack->stack_idx = stack_idx;

    handler->operations.write(out_package_stack, interface, handler);
}

uint16_t tcp_close_wait(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    struct tcp_block_t* incoming_packets = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->in_buffer);
    
    while(num_ready--) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                // If the RST bit is set, then any outstanding RECEIVEs and SEND should receive "reset" responses. 
                // All segment queues should be flushed. Users should also receive an unsolicited general "connection reset" signal. 
                // Enter the CLOSED state, delete the TCB, and return.
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 0;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) {                
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);
                return 0;
            } else if(tcp_header->control_bits & TCP_ACK_FLAG) {
                // we do not check for the RFC 5961 blind data injection 
                // attack problem mentioned in RFC 9293        

                uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
                uint32_t sequence_num = ntohl(tcp_header->sequence_num);        

                if(is_acknowledgement_valid(tcb, tcp_header) || tcb->send_unacknowledged == acknowledgement_num) {                                        
                    tcb->send_unacknowledged = acknowledgement_num;

                    // acknowledgement of something which has not yet been send
                    if(acknowledgement_num > tcb->send_next && tcb->send_unacknowledged != acknowledgement_num) {
                        tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                        return 0;
                    }

                    // update window if this is not an old segment
                    if(tcb->send_last_update_sequence_num < sequence_num ||
                      (tcb->send_last_update_sequence_num == sequence_num 
                        && tcb->send_last_update_acknowledgement_num <= acknowledgement_num)) {
                        
                        tcb->send_window = ntohs(tcp_header->window);
                        tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                        tcb->send_last_update_acknowledgement_num = tcb->send_last_update_acknowledgement_num;
                    }                      
                }
            } 
        } else {
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else {
                // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
            }                
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }

    return 0;
}


uint16_t tcp_established(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    struct tcp_block_t* incoming_packets = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->in_buffer);
    
    while(num_ready--) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                // If the RST bit is set, then any outstanding RECEIVEs and SEND should receive "reset" responses. 
                // All segment queues should be flushed. Users should also receive an unsolicited general "connection reset" signal. 
                // Enter the CLOSED state, delete the TCB, and return.
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 0;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) {                
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);
                return 0;
            } else if(tcp_header->control_bits & TCP_ACK_FLAG) {
                // we do not check for the RFC 5961 blind data injection 
                // attack problem mentioned in RFC 9293        

                uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
                uint32_t sequence_num = ntohl(tcp_header->sequence_num);        

                if(is_acknowledgement_valid(tcb, tcp_header) || tcb->send_unacknowledged == acknowledgement_num) {                                        
                    tcb->send_unacknowledged = acknowledgement_num;

                    // acknowledgement of something which has not yet been send
                    if(acknowledgement_num > tcb->send_next && tcb->send_unacknowledged != acknowledgement_num) {
                        tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                        return 0;
                    }

                    // update window if this is not an old segment
                    if(tcb->send_last_update_sequence_num < sequence_num ||
                      (tcb->send_last_update_sequence_num == sequence_num 
                        && tcb->send_last_update_acknowledgement_num <= acknowledgement_num)) {
                        
                        tcb->send_window = ntohs(tcp_header->window);
                        tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                        tcb->send_last_update_acknowledgement_num = tcb->send_last_update_acknowledgement_num;
                    }

                    // if the package contains a payload, pass it to next handler
                    if(payload_size) {                        
                        struct handler_t* next_handler = socket->next_handler;

                        tcb->receive_next += payload_size;
                        tcb->receive_window -= payload_size;
                        
                        tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);

                        // fill in write arguments for possible future write based on this read
                        struct tcp_write_args_t tcp_args = {
                            .connection_id = tcb->id,
                            .socket = socket,
                            .flags = TCP_ACK_FLAG
                        };
                        packet_stack->return_args[packet_stack->stack_idx] = &tcp_args;

                        // set next buffer pointer for next protocol level     
                        packet_stack->stack_idx++;
                        packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx] = tcp_get_payload(tcp_header);                        

                        // call next protocol level, tls, http, app specific etc.
                        socket->next_handler->operations.read(packet_stack, interface, next_handler);
                    }                        
                }
            } 
        } else {
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else {
                // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
            }                
        }

        
        if(tcp_header->control_bits & TCP_FIN_FLAG) {
            tcb->receive_next = ntohl(tcp_header->sequence_num) + 1;       

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG | TCP_FIN_FLAG, packet_stack->stack_idx, interface);
            
            tcb->state = CLOSE_WAIT;
            tcb->state_function = tcp_close_wait;
            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
            
            return tcb->state_function(handler, tcb, num_ready, interface);
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }

    return 0;
}

uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);

uint16_t tcp_syn_received(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) { 
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);
                return 0;
            } else if(tcp_header->control_bits & TCP_ACK_FLAG) {
                // we do not check for the RFC 5961 blind data injection 
                // attack problem mentioned in RFC 9293                
                if(!is_acknowledgement_valid(tcb, tcp_header)) {
                    tcb->send_next = ntohl(tcp_header->acknowledgement_num);
                    
                    tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_RST_FLAG, packet_stack->stack_idx, interface);
                } else {
                    tcb->send_window = ntohs(tcp_header->window);
                    tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                    tcb->send_last_update_acknowledgement_num = ntohl(tcp_header->acknowledgement_num);

                    tcb->state = ESTABLISHED;
                    tcb->state_function = tcp_established;
                    return tcb->state_function(handler, tcb, num_ready, interface);
                }
            } 
        } else {
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else {
                // <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
            }                
        }

        if(tcp_header->control_bits & TCP_FIN_FLAG) {
            tcb->receive_next = ntohl(tcp_header->sequence_num) + 1;       

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG | TCP_FIN_FLAG, packet_stack->stack_idx, interface);
            
            tcb->state = CLOSE_WAIT;
            tcb->state_function = tcp_close_wait;

            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  

            return 0;
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }
    return 1;
}


uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);

        if(tcp_header->control_bits & TCP_RST_FLAG) {            
            tcp_delete_transmission_control_block(handler, socket, tcb->id);
            return 0;
        }
        else if(tcp_header->control_bits & TCP_ACK_FLAG) {   
            tcb->send_next = ntohl(tcp_header->acknowledgement_num);         
            tcb->receive_next = 0;

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_RST_FLAG, packet_stack->stack_idx, interface);
        } else if(tcp_header->control_bits | TCP_SYN_FLAG == TCP_SYN_FLAG) {
            tcb->receive_initial_sequence_num = ntohl(tcp_header->sequence_num);           
            tcb->receive_next = tcb->receive_initial_sequence_num + 1; 
            tcb->receive_urgent_pointer = 0;

            tcb->state = SYN_RECEIVED;
            tcb->state_function = tcp_syn_received;

            tcb->send_next = tcb->send_initial_sequence_num;

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG | TCP_SYN_FLAG, packet_stack->stack_idx, interface);
            
            tcb->send_unacknowledged = tcb->send_initial_sequence_num;
            tcb->send_next++;      
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);         
    }

    return 0;
}

bool tcp_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct tcp_write_args_t* tcp_args = (struct tcp_write_args_t*) packet_stack->args[packet_stack->stack_idx];

    if(!packet_stack->stack_idx) {
        NETSTACK_LOG(NETSTACK_WARNING, "TCP as the bottom header in packet stack is not possible, yet it happended.\n");   
        return false;
    }

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    struct tcp_header_t* response_header = (struct tcp_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct tcp_header_t)); // we don't calculate in options here

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
    out_header->source_port = tcp_args->socket->listening_port;
    out_header->destination_port = tcb->remote_port;
    out_header->data_offset = (sizeof(struct tcp_header_t) / 4) << 4; // data offset is counted in 32 bit chunks 
    out_header->window = htons(tcb->receive_window);
    out_header->urgent_pointer = 0; // Not supported
    out_header->control_bits = tcp_args->flags;

    out_header->sequence_num = htonl(tcb->send_next);
    out_header->acknowledgement_num = htonl(tcb->receive_next);

    if(payload_size) {
        out_header->control_bits |= TCP_PSH_FLAG;
        tcb->send_next += payload_size;        
    }

    out_header->checksum = _tcp_calculate_checksum(out_header, tcp_args->socket->ipv4, tcb->remote_ipv4);
    
    memcpy(response_header, tcb->out_header, sizeof(struct tcp_header_t));
    out_buffer->offset -= sizeof(struct tcp_header_t);
   
    // add buffer to outgoing block buffer
    tcp_block_buffer_add(tcb->out_buffer, packet_stack, out_header->sequence_num, payload_size);
        
    // then iterate over all outgoin buffers 
    struct tcp_block_t* buffer_block;
    while(buffer_block = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->out_buffer)) {
        struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) buffer_block->data;
    
        out_buffer = &out_package_stack->out_buffer;
        response_header = (struct tcp_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct tcp_header_t)); // we don't calculate in options here
        
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
                header, ipv4_header->source_ip, tcp_listen);
            
                if(!tcb) {
                    handler->handler_config->mem_free(tcb);
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
        
        uint16_t num_ready = tcp_block_buffer_num_ready(tcb->in_buffer, tcb->receive_next);
        
        if(num_ready) {
            return tcb->state_function(handler, tcb, num_ready, interface);
        }
    }

    return 0;
}

struct handler_t* tcp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("tcp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = tcp_init_handler;
    handler->close = tcp_close_handler;

    handler->operations.read = tcp_read;
    handler->operations.write = tcp_write;        

    ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, handler); 

    return handler;
}