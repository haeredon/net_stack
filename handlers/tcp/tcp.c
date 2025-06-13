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
    uint64_t id = ((uint64_t) ipv4_header->source_ip) << 32;
    id |= ((uint32_t) tcp_header->source_port) << 16;
    id |= tcp_header->destination_port;
    return id;
}

bool tcp_internal_write(struct handler_t* handler, struct tcp_socket_t* socket, uint32_t connection_id, uint8_t stack_idx, struct interface_t* interface) {

    struct new_out_packet_stack_t* out_packate_stack = (struct new_out_packet_stack_t*) handler->handler_config->
                mem_allocate("response: tcp_package", DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct new_out_packet_stack_t)); 

    out_packate_stack->out_buffer.buffer = (uint8_t*) out_packate_stack + sizeof(struct new_out_packet_stack_t);
    out_packate_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_packate_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;      

    struct tcp_write_args_t args = {
        .connection_id = connection_id,
        .socket = socket
    };

    handler->operations.write(out_packate_stack, &args, stack_idx, interface, handler);
}

void set_response(struct tcp_header_t* response_header, uint32_t sequence_num, uint32_t acnowledgement_num, uint8_t control_bits) {
    response_header->sequence_num = sequence_num;                         
    response_header->acknowledgement_num = acnowledgement_num;               
    response_header->control_bits = control_bits;            
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


uint16_t tcp_established(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    struct tcp_block_t* incoming_packets = (struct tcp_block_t*) tcp_block_buffer_get_head(tcb->in_buffer);
    
    while(num_ready--) {
        struct packet_stack_t* packet_stack = (struct packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];

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
                set_response(
                        (struct tcp_header_t*) tcb->out_header,
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

                    // acknowledgement of something which has not yet been send
                    if(acknowledgement_num > tcb->send_next && tcb->send_unacknowledged != acknowledgement_num) {
                        set_response(
                            (struct tcp_header_t*) tcb->out_header,
                            htonl(tcb->send_next),
                            htonl(tcb->receive_next),
                            TCP_ACK_FLAG
                        );
                        tcp_internal_write(handler, socket, tcb->id, packet_stack->write_chain_length, interface);
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

                        set_response(
                            (struct tcp_header_t*) tcb->out_header,
                            htonl(tcb->send_next),
                            htonl(tcb->receive_next),
                            TCP_ACK_FLAG
                        );        

                        tcp_internal_write(handler, socket, tcb->id, packet_stack->write_chain_length, interface);

                        // set next buffer pointer for next protocol level     
                        packet_stack->write_chain_length++;
                        packet_stack->packet_pointers[packet_stack->write_chain_length] = tcp_get_payload(tcp_header);                        

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
                set_response(
                    (struct tcp_header_t*) tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG
                );

                tcp_internal_write(handler, socket, tcb->id, packet_stack->write_chain_length, interface);
            }                
        }

        
        if(tcp_header->control_bits & TCP_FIN_FLAG) {
            tcb->receive_next = ntohl(tcp_header->sequence_num) + 1;       

            set_response(
                    (struct tcp_header_t*) tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG & TCP_FIN_FLAG
            );      

            tcb->state = CLOSE_WAIT;
            tcb->state_function = tcp_close_wait;
            tcp_block_buffer_remove_front(tcb->in_buffer, 1);  

            return 0;
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);  
    }

    return 0;
}

uint16_t tcp_listen(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface);

uint16_t tcp_syn_received(struct handler_t* handler, struct transmission_control_block_t* tcb, uint16_t num_ready, struct interface_t* interface) {
    if(num_ready) {
        struct packet_stack_t* packet_stack = (struct packet_stack_t*) tcp_block_buffer_get_head(tcb->in_buffer)->data;

        struct tcp_header_t* tcp_header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length];
        struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_stack->write_chain_length - 1];

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);
    
        if(is_segment_in_window(tcb, tcp_header, payload_size)) {
            
            if(tcp_header->control_bits & TCP_RST_FLAG) {
                tcp_delete_transmission_control_block(handler, socket, tcb->id);
                return 1;
            } else if(tcp_header->control_bits & TCP_SYN_FLAG) {
                set_response(
                        (struct tcp_header_t*) tcb->out_header,
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
                        (struct tcp_header_t*) tcb->out_header,
                        htonl(tcp_header->acknowledgement_num),
                        htonl(tcb->receive_next),
                        TCP_RST_FLAG
                    );

                    tcp_internal_write(handler, socket, tcb->id, packet_stack->write_chain_length, interface);
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
                    (struct tcp_header_t*) tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG
                );

                tcp_internal_write(handler, socket, tcb->id, packet_stack->write_chain_length, interface);
            }                
        }

        if(tcp_header->control_bits & TCP_FIN_FLAG) {
            tcb->receive_next = ntohl(tcp_header->sequence_num) + 1;       

            set_response(
                    (struct tcp_header_t*) tcb->out_header,
                    htonl(tcb->send_next),
                    htonl(tcb->receive_next),
                    TCP_ACK_FLAG & TCP_FIN_FLAG
            );      

            tcb->state = CLOSE_WAIT;
            tcb->state_function = tcp_close_wait;
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

        struct tcp_socket_t* socket = tcp_get_socket(handler, ipv4_header->destination_ip, tcp_header->destination_port);

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

            tcp_internal_write(handler, packet_stack->write_chain_length, interface);
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

            tcp_internal_write(handler, packet_stack->write_chain_length, interface);
        }

        tcp_block_buffer_remove_front(tcb->in_buffer, 1);         
    }

    return 0;
}

bool tcp_write(struct new_out_packet_stack_t* packet_stack, void* args, struct interface_t* interface, const struct handler_t* handler) {
    struct tcp_write_args_t* tcp_args = (struct tcp_write_args_t*) args;

    if(!packet_stack->stack_idx) {
        NETSTACK_LOG(NETSTACK_WARNING, "TCP as the bottom header in packet stack is not possible, yet it happended.\n");   
        return false;
    }

    struct new_out_buffer_t out_buffer = packet_stack->out_buffer;
    struct tcp_header_t* response_header = (struct tcp_header_t*) ((uint8_t*) out_buffer.buffer + out_buffer.offset - sizeof(struct tcp_header_t)); // we don't calculate in options here

    uint64_t payload_size = out_buffer.size - out_buffer.offset;

    if(out_buffer.offset < sizeof(struct tcp_header_t)) { // we don't calculate in options here
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

    struct transmission_control_block_t* tcb = tcp_get_transmission_control_block(socket, tcp_args->connection_id);

    if(!tcb) {
        NETSTACK_LOG(NETSTACK_WARNING, "Could not find transmission control block.\n");   
        return false;
    }

    struct tcp_header_t* out_header = (struct tcp_header_t*) tcb->out_header;
    out_header->source_port = tcp_args->socket->ipv4;
    out_header->destination_port = tcb->remote_ipv4;
    out_header->data_offset = (sizeof(struct tcp_header_t) / 4) << 4; // data offset is counted in 32 bit chunks 
    out_header->window = htons(tcb->receive_window);
    out_header->urgent_pointer = 0; // Not supported

    out_header->checksum = _tcp_calculate_checksum(out_header, tcp_args->socket->ipv4, tcb->remote_ipv4);
    
    memcpy(response_header, tcb->out_header, sizeof(struct tcp_header_t));
    out_buffer.offset -= sizeof(struct tcp_header_t);
   
    // add buffer to outgoing block buffer
    tcp_block_buffer_add(tcb->out_buffer, out_buffer.buffer, out_header->sequence_num, payload_size);
        
    // then iterate over all outgoin buffers 
    uint16_t num_ready = tcp_block_buffer_num_ready(tcb->out_buffer,);
    while(num_ready--) {
        struct package_buffer_t* buffer = (struct package_buffer_t*) tcp_block_buffer_get_head(tcb->out_buffer)->data;
    
        // if the outgoing package fit in the send window then send it, else just abort but keep the buffer for later
        if(fit in window) {
            // this can't be the bottom if the stack because TCP is dependent on IP, hence no check for the bottom
            const struct handler_t* next_handler = packet_stack->handlers[--stack_idx];
            
            if(!next_handler->operations.write(packet_stack, buffer, stack_idx, interface, next_handler)) {
                return false;
            }

            tcp_block_buffer_remove_front(tcb->out_buffer, 1);         
        } else {
            break;
        }        
    }

    return true
    
}

uint16_t tcp_read(struct new_in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) { 
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