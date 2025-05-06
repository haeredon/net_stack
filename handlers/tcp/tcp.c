#include "tcp.h"
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
 * No support for urgent pointer, options and checksum validation
 * 
 * Buffer overflow dangers:     
 *      1. Response buffer size
 *      2. Number of connections
 *      3. Number of listeners
 */


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
    uint64_t id = ((uint64_t) ipv4_header->source_ip) << 32;
    id |= ((uint32_t) tcp_header->source_port) << 16;
    id |= tcp_header->destination_port;
    return id;
}

uint16_t tcp_handle_pre_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface) { 
    struct ipv4_header_t* ipv4_request_header = (struct ipv4_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx - 1];
    struct tcp_header_t* tcp_request_header = (struct tcp_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct tcp_header_t* tcp_response_buffer = (struct tcp_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    struct tcp_socket_t* socket = tcp_get_socket(packet_stack->handlers[response_buffer->stack_idx], ipv4_request_header->destination_ip, tcp_request_header->destination_port);

    if(!socket) {
        NETSTACK_LOG(NETSTACK_ERROR, "Could not find socket.\n");   
    }

    uint32_t connection_id = tcb_header_to_connection_id(ipv4_request_header, tcp_request_header);    

    if(!connection_id) {
        NETSTACK_LOG(NETSTACK_ERROR, "Could not find connection id.\n");   
    }

    struct transmission_control_block_t* tcb = tcp_get_transmission_control_block(socket, connection_id);

    if(!tcb) {
        NETSTACK_LOG(NETSTACK_ERROR, "Could not find transmission control block.\n");   
    }

    struct tcp_header_t* out_header = (struct tcp_header_t*) tcb->out_header;
    out_header->source_port = tcp_request_header->destination_port;
    out_header->destination_port = tcp_request_header->source_port;
    out_header->data_offset = (sizeof(struct tcp_header_t) / 4) << 4; // data offset is counted in 32 bit chunks 
    out_header->window = htons(tcb->receive_window);
    out_header->urgent_pointer = 0; // Not supported

    out_header->checksum = _tcp_calculate_checksum(out_header, socket->ipv4, ipv4_request_header->source_ip);

    memcpy(tcp_response_buffer, tcb->out_header, sizeof(struct tcp_header_t));

    uint16_t num_bytes_written = sizeof(struct tcp_header_t);    
    response_buffer->offset += num_bytes_written;

    return num_bytes_written;
}

uint16_t tcp_time_wait(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    return 1;
}

uint16_t tcp_last_ack(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    return 1;
}

uint16_t tcp_closing(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    return 1;
}

uint16_t tcp_close_wait(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    return 1;
}

void set_transmission_control_block_send_window(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header) {
    uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
    uint32_t sequence_num = ntohl(tcp_header->sequence_num);
    
    // update window if this is not an old segment
    if(tcb->send_last_update_sequence_num < sequence_num ||
      (tcb->send_last_update_sequence_num == sequence_num 
      && tcb->send_last_update_acknowledgement_num <= acknowledgement_num)) {
                        
        tcb->send_window = ntohs(tcp_header->window);
        tcb->send_last_update_sequence_num = tcp_header->sequence_num;
        tcb->send_last_update_acknowledgement_num = tcb->send_last_update_acknowledgement_num;
    }
}

uint16_t tcp_established(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    struct tcp_block_t* incoming_packets = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->in_buffer);
    
    while(num_ready--) {
        struct packet_stack_t* packet_stack = (struct packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        packet_stack->pre_build_response[packet_stack->write_chain_length] = tcp_handle_pre_response;
        packet_stack->write_chain_length++;
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                // If the RST bit is set, then any outstanding RECEIVEs and SEND should receive "reset" responses. 
                // All segment queues should be flushed. Users should also receive an unsolicited general "connection reset" signal. 
                // Enter the CLOSED state, delete the TCB, and return.
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 0;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) {
                set_response(
                        tcb->out_header,
                        htonl(tcb->send_next),
                        htonl(tcb->receive_next),
                        TCP_ACK_FLAG
                );
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);
                return 0;
            } else if(tcp_header->control_bits & TCP_ACK_FLAG) {
                // we do not check for the RFC 5961 blind data injection 
                // attack problem mentioned in RFC 9293        

                uint32_t acknowledgement_num = ntohl(tcp_header->acknowledgement_num);
                uint32_t sequence_num = ntohl(tcp_header->sequence_num);        

                if(is_acknowledgement_valid(tcb, tcp_header) || tcb->send_unacknowledged == acknowledgement_num) {                    
                    tcb->send_unacknowledged = acknowledgement_num;

                    // acknowledgement of something which has already been acknowledged
                    if(acknowledgement_num <= tcb->send_unacknowledged) {
                        return 0;
                    }

                    // acknowledgement of something which has not yet been send
                    if(acknowledgement_num > tcb->send_next && tcb->send_unacknowledged != acknowledgement_num) {
                        set_response(
                            tcb->out_header,
                            htonl(tcb->send_next),
                            htonl(tcb->receive_next),
                            TCP_ACK_FLAG
                        );
                        handler->handler_config->write(packet_stack, interface, 0);                
                        return 0;
                    }

                    // update window if this is not an old segment
                    if(tcb->send_last_update_sequence_num < sequence_num ||
                      (tcb->send_last_update_sequence_num == sequence_num 
                        && tcb->send_last_update_acknowledgement_num <= acknowledgement_num)) {
                        
                        tcb->send_window = ntohs(tcp_header->window);
                        tcb->send_last_update_sequence_num = tcp_header->sequence_num;
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
                set_response(
                    tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG
                );

                handler->handler_config->write(packet_stack, interface, 0);                
            }                
        }

        // IF (DATA) THEN DEAL WITH TEXT SEGMENT HERE

        if(tcp_header->control_bits & TCP_FIN_FLAG) {

        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }
    return 1;
}

void set_response(struct tcp_header_t* response_header, uint32_t sequence_num, uint32_t acnowledgement_num, uint8_t control_bits) {
    response_header->sequence_num = sequence_num;                         
    response_header->acknowledgement_num = acnowledgement_num;               
    response_header->control_bits = TCP_ACK_FLAG | TCP_SYN_FLAG;            
}

uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);

uint16_t tcp_syn_received(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct packet_stack_t* packet_stack = (struct packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        packet_stack->pre_build_response[packet_stack->write_chain_length] = tcp_handle_pre_response;
        packet_stack->write_chain_length++;
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) {
                set_response(
                        tcb->out_header,
                        htonl(tcb->send_next),
                        htonl(tcb->receive_next),
                        TCP_ACK_FLAG
                );
                tcp_block_buffer_remove_front(tcb->in_buffer, 1);
                return 0;
            } else if(tcp_header->control_bits & TCP_ACK_FLAG) {
                // we do not check for the RFC 5961 blind data injection 
                // attack problem mentioned in RFC 9293                
                if(!is_acknowledgement_valid(tcb, tcp_header)) {
                    set_response(
                        tcb->out_header,
                        htonl(tcp_header->acknowledgement_num),
                        htonl(tcb->receive_next),
                        TCP_RST_FLAG
                    );

                    handler->handler_config->write(packet_stack, interface, 0);
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
                set_response(
                    tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG
                );

                handler->handler_config->write(packet_stack, interface, 0);                
            }                
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }
    return 1;
}



uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct packet_stack_t* packet_stack = (struct packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];

        packet_stack->pre_build_response[packet_stack->write_chain_length] = tcp_handle_pre_response;
        packet_stack->write_chain_length++;

        if(tcp_header->control_bits & TCP_RST_FLAG) {            
            tcp_delete_transmission_control_block(handler, socket, tcb->id);
            return 0;
        }
        else if(tcp_header->control_bits & TCP_ACK_FLAG) {            
            set_response(
                (struct tcp_header_t*) tcb->out_header,
                tcp_header->acknowledgement_num,
                0,
                TCP_RST_FLAG
            );
        
            handler->handler_config->write(packet_stack, interface, 0);
        } else if(tcp_header->control_bits | TCP_SYN_FLAG == TCP_SYN_FLAG) {
            tcb->receive_initial_sequence_num = ntohl(tcp_header->sequence_num);           
            tcb->receive_next = tcb->receive_initial_sequence_num + 1; 
            tcb->receive_urgent_pointer = 0;

            tcb->state = SYN_RECEIVED;
            tcb->state_function = tcp_syn_received;
            
            set_response(
                (struct tcp_header_t*) tcb->out_header,
                htonl(tcb->send_initial_sequence_num),
                htonl(tcb->receive_next),
                TCP_ACK_FLAG | TCP_SYN_FLAG
            );
            
            tcb->send_unacknowledged = tcb->send_initial_sequence_num;
            tcb->send_next++;            

            handler->handler_config->write(packet_stack, interface, 0);
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);         
    }

    return 0;
}

uint16_t tcp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) { 
    packet_stack->handlers[packet_stack->write_chain_length] = handler;  
    struct tcp_header_t* header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
    
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];
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

    ADD_TO_PRIORITY(&ip_type_to_handler, 0x06, handler); 

    return handler;
}