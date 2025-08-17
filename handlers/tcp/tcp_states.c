
#include "handlers/tcp/tcp_states.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp_shared.h"
#include "util/log.h"
#include "tcp_block_buffer.h"
#include "tcp_shared.h"
#include "socket.h"

#include <string.h>
#include <arpa/inet.h>


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

uint16_t tcp_close(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    while(num_ready--) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);
    
        if(!(tcp_header->control_bits & TCP_RST_FLAG)) {
            uint8_t control_bits = TCP_RST_FLAG;

            if(tcp_header->control_bits & TCP_ACK_FLAG) {
                tcb->send_next = ntohl(tcp_header->acknowledgement_num);
            } else {
                tcb->send_next = 0;
                tcb->receive_next = ntohl(tcp_header->sequence_num) + payload_size;
                control_bits |= TCP_ACK_FLAG;
            }

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_RST_FLAG, packet_stack->stack_idx, interface);
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }

    return 0;
}

void tcp_receive_payload(struct handler_t* handler, struct tcp_socket_t* socket, 
    struct transmission_control_block_t* tcb, struct in_packet_stack_t* packet_stack, 
    struct interface_t* interface,
    void* payload, uint16_t payload_size) {                       
    
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
    packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx] = payload;                        

    // call next protocol level, tls, http, app specific etc.
    socket->next_handler->operations.read(packet_stack, interface, next_handler);
}

uint16_t tcp_syn_sent(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);

        bool ack_acceptable = false;

        if(tcp_header->control_bits & TCP_ACK_FLAG) {            
            uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);

            if(acknowledgement_num <= tcb->send_initial_sequence_num ||
               acknowledgement_num > tcb->send_next) {
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_RST_FLAG, packet_stack->stack_idx, interface);
                return 0;
            } 

            ack_acceptable = true;
        } 
        
        if(tcp_header->control_bits & TCP_RST_FLAG) {
            if(ack_acceptable) {                
                // signal user that connection has been terminated
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 0;
            } else {
                tcp_block_buffer_remove_front(tcb->in_buffer, 1); 
                return 0;
            }
        }

        if(tcp_header->control_bits & TCP_SYN_FLAG) {
            tcb->receive_initial_sequence_num = ntohl(tcp_header->sequence_num);
            tcb->receive_next = tcb->receive_initial_sequence_num + 1; 

            if(tcp_header->control_bits & TCP_ACK_FLAG) {
                tcb->send_unacknowledged = ntohl(tcp_header->acknowledgement_num);
            }

            if(tcb->send_unacknowledged > tcb->send_initial_sequence_num) {                
                tcb->state = ESTABLISHED;
                tcb->state_function = tcp_others;                

                uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);
                
                if(payload_size) {
                    tcp_receive_payload(handler, socket, tcb, packet_stack, interface, tcp_get_payload(tcp_header), payload_size);
                } else {
                    tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                }
            } else {
                tcb->state = SYN_RECEIVED;
                tcb->state_function = tcp_others;

                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_SYN_FLAG | TCP_ACK_FLAG, packet_stack->stack_idx, interface);

                tcb->send_window = ntohl(tcp_header->window);
                tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                tcb->send_last_update_acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
            }
        }

        if((!tcp_header->control_bits & (TCP_SYN_FLAG | TCP_RST_FLAG))) {
            tcp_block_buffer_remove_front(tcb->in_buffer, 1); 
        }

        return 0;
    }

    return 1;
}


uint16_t tcp_others(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
     while(num_ready--) {
        struct in_packet_stack_t* packet_stack = (struct in_packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->in_buffer.packet_pointers[packet_stack->stack_idx - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);

        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            enum TCP_STATE state = tcb->state;
        
            if(tcp_header->control_bits & TCP_RST_FLAG) {                                
                if(state == SYN_RECEIVED) {
                    if(tcb->active_mode) {
                        socket->operations.on_close();                      
                    } 
                    tcb->state == CLOSED;
                    return 0;                 
                } else if(state == CLOSING || state == LAST_ACK || state ==  TIME_WAIT) {
                    tcb->state == CLOSED;
                    return 0;        
                } else {
                    // any outstanding RECEIVEs and SEND should receive "reset" responses. 
                    // All segment queues should be flushed. Users should also receive an
                    // unsolicited general "connection reset" signal.
                    tcb->state == CLOSED;
                    return 0;  
                }
            }

            if(tcp_header->control_bits & TCP_SYN_FLAG) {   
                if(state == SYN_RECEIVED && !tcb->active_mode) {
                    tcb->state == CLOSED;
                    return 0;
                } else {
                    tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                    tcp_block_buffer_remove_front(tcb->in_buffer, 1);                
                    continue;
                }
            }

            if(tcp_header->control_bits & TCP_ACK_FLAG) {
                uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
                uint32_t sequence_num = ntohl(tcp_header->sequence_num);       

                bool ackOk = acknowledgement_num <= tcb->send_next && acknowledgement_num > tcb->send_unacknowledged;

                if(state == ESTABLISHED || state == FIN_WAIT_1 || state == FIN_WAIT_2 || state == CLOSE_WAIT || state == CLOSING) {                    
                    // acknowledgement of something which has not yet been send
                    if(acknowledgement_num > tcb->send_next) {
                        tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                        tcp_block_buffer_remove_front(tcb->in_buffer, 1);                
                        continue;
                    }

                    if(ackOk || acknowledgement_num == tcb->send_unacknowledged) {
                        // update window if this is not an old segment
                        if(tcb->send_last_update_sequence_num < sequence_num ||
                        (tcb->send_last_update_sequence_num == sequence_num 
                            && tcb->send_last_update_acknowledgement_num <= acknowledgement_num)) {
                            
                            tcb->send_window = ntohs(tcp_header->window);
                            tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                            tcb->send_last_update_acknowledgement_num = tcb->send_last_update_acknowledgement_num;
                        }
                    }
                    
                    if(ackOk) {
                        tcb->send_unacknowledged = acknowledgement_num;
                    }

                    if(state == FIN_WAIT_1) {
                        // if this is an acknowledgement of a previous sent FIN
                        if(tcb->fin_num && acknowledgement_num >= tcb->fin_num) {
                            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
                            tcb->state = FIN_WAIT_2;
                        }
                    } else if(state == FIN_WAIT_2) {
                        // if the retransmission queue is empty, then report a close to the user 
                        if(tcb->send_next == tcb->send_last_update_acknowledgement_num) {
                            socket->operations.on_close();
                        }
                    } else if(state == CLOSING) {
                        // if this is an acknowledgement of a previous sent FIN
                        if(tcb->fin_num && acknowledgement_num >= tcb->fin_num) {
                            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
                            tcb->state = TIME_WAIT;
                        }
                    } else if(state == LAST_ACK) {
                        // if this is an acknowledgement of a previous sent FIN
                        if(tcb->fin_num && acknowledgement_num >= tcb->fin_num) {
                            tcb->state == CLOSED;
                            return 0;
                        }
                    } else if(state == TIME_WAIT) {
                        // if this is an acknowledgement of a previous sent FIN
                        if(tcb->fin_num && acknowledgement_num >= tcb->fin_num) {
                            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
                            // TODO: restart timer
                        }                        
                    } 
                } else if(state == SYN_RECEIVED) {
                    if(!ackOk) {
                        tcb->send_next = ntohl(tcp_header->acknowledgement_num);                            
                        tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_RST_FLAG, packet_stack->stack_idx, interface);
                    } else {
                        tcb->send_window = ntohs(tcp_header->window);
                        tcb->send_last_update_sequence_num = ntohl(tcp_header->sequence_num);
                        tcb->send_last_update_acknowledgement_num = ntohl(tcp_header->acknowledgement_num);

                        tcb->state = ESTABLISHED;
                    }
                }
            } else {
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);                
                continue;
            }

            if(payload_size) {
                if(state == ESTABLISHED || state == FIN_WAIT_1 || state == FIN_WAIT_2) {
                    tcp_receive_payload(handler, socket, tcb, packet_stack, interface, tcp_get_payload(tcp_header), payload_size);                    
                }
            }
            
            if(tcp_header->control_bits & TCP_FIN_FLAG) {
                if(state == CLOSED || state == LISTEN || state == SYN_SENT) {
                    tcp_block_buffer_remove_front(tcb->in_buffer, 1);                
                    continue;
                }
                
                tcb->receive_next = ntohl(tcp_header->sequence_num) + 1;
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG | TCP_FIN_FLAG, packet_stack->stack_idx, interface);

                tcb->send_next++;

                socket->operations.on_close();

                if(state == ESTABLISHED) {
                    tcb->state = CLOSE_WAIT;             
                } else if(state == FIN_WAIT_1) {
                    uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
                    // if this is an acknowledgement of a previous sent FIN
                    if(tcb->fin_num && acknowledgement_num >= tcb->fin_num) {
                        tcb->state = TIME_WAIT; 
                    } else {
                        tcb->state = CLOSING;
                    }
                } else if(state == FIN_WAIT_2) {
                    tcb->state = TIME_WAIT;
                } else if(state == TIME_WAIT) {
                    // TODO: restart timer
                } 
            }
        } else {
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
                continue;
            } else {
                tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG, packet_stack->stack_idx, interface);
            }                
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }
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
            tcb->state_function = tcp_others;

            tcb->send_next = tcb->send_initial_sequence_num;

            tcp_internal_write(handler, packet_stack, socket, tcb->id, TCP_ACK_FLAG | TCP_SYN_FLAG, packet_stack->stack_idx, interface);
            
            tcb->send_unacknowledged = tcb->send_initial_sequence_num;
            tcb->send_next++;      
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);         
    }

    return 0;
}