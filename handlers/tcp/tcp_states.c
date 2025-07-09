
#include "handlers/tcp/tcp_states.h"
#include "handlers/tcp/tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp_shared.h"
#include "log.h"
#include "tcp_block_buffer.h"
#include "tcp_shared.h"
#include "socket.h"

#include <string.h>
#include <arpa/inet.h>





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
    NETSTACK_LOG(NETSTACK_ERROR, "TCP: tcp_close() not implemented.\n");   
    return -1;
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
            
            tcb->state_function = tcp_close_wait;
            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
            
            return tcb->state_function(handler, tcb, num_ready, interface);
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }

    return 0;
}

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