#include "tcp.h"
#include "ipv4.h"
#include "log.h"

#include <string.h>

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

// crude, crude, crude implementation of transmission block buffer
#define TRANSMISSION_CONTROL_BLOCK_BUFFER_SIZE 64
struct transmission_control_block_t* transmission_blocks[TRANSMISSION_CONTROL_BLOCK_BUFFER_SIZE];


void tcp_close_handler(struct handler_t* handler) {
    struct tcp_priv_t* private = (struct tcp_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void tcp_init_handler(struct handler_t* handler) {
    struct tcp_priv_t* tcp_priv = (struct tcp_priv_t*) handler->handler_config->mem_allocate("tcp handler private data", sizeof(struct tcp_priv_t)); 
    handler->priv = (void*) tcp_priv;

    memset(transmission_blocks, 0, sizeof(transmission_blocks));
}


uint16_t tcp_calculate_checksum()  {
    // uint32_t sum = 0;
    // uint16_t length = be16toh(ip_pseudo_header.tcp_length) / 2;

    // uint16_t* ip = (uint16_t*) &ip_pseudo_header;
    // for(uint16_t i = 0 ; i < 6 ; ++i) {
    //     sum += ip[i];
    // }

    // for(uint16_t i = 0 ; i < length ; ++i) {
    //     sum += tcp[i];
    // }

    // while (sum>>16) {
    //     sum = (sum & 0xffff) + (sum >> 16);
    // }

    // return (uint16_t) ~sum;
}



uint16_t tcp_syn_received(struct tcp_header_t* request_header, struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {
    // respond and change state to established
    return 1;
}

uint16_t tcp_listen(struct tcp_header_t* request_header, struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {
    tcb->receive_initial_sequence_num = request_header->sequence_num;
    tcb->receive_next = tcb->receive_initial_sequence_num + 1;
    tcb->receive_window = request_header->window;
    tcb->receive_urgent_pointer = 0;

    tcb->state = SYN_RECEIVED;
    tcb->state_function = tcp_syn_received;

    response_buffer->source_port = request_header->destination_port;
    response_buffer->destination_port = request_header->source_port;
    response_buffer->sequence_num = tcb->send_next;
    response_buffer->acknowledgement_num = tcb->receive_next;
    response_buffer->data_offset = sizeof(struct tcp_header_t) / 32; // data offset is counted in 32 bit chunks 
    response_buffer->control_bits = TCP_ACK_FLAG | TCP_SYN_FLAG;
    response_buffer->window = tcb->send_window;
    response_buffer->checksum = tcp_calculate_checksum();
    response_buffer->urgent_pointer = 0; // Not supported

    tcb->send_next++;

    return sizeof(struct tcp_header_t);
}

uint16_t tcp_established(struct tcp_header_t* request_header, struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {
    // do whatever
    return 1;
}


uint32_t tcp_generate_sequence_number() {
    return 42;
}

uint32_t tcb_header_to_connection_id(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    uint64_t id = ((uint64_t) ipv4_header->source_ip) << 32;
    id |= ((uint32_t) tcp_header->source_port) << 16;
    id |= tcp_header->destination_port;
    return id;
}

struct transmission_control_block_t* get_transmission_control_block(uint32_t id) {
    for (uint8_t i = 0 ; i < TRANSMISSION_CONTROL_BLOCK_BUFFER_SIZE ; i++) {
        struct transmission_control_block_t* tcb = transmission_blocks[i];
        
        if(tcb && tcb->id == id) {
            return tcb;
        } 
    }   
    return 0; 
}

struct transmission_control_block_t* create_transmission_control_block(uint32_t connection_id, 
    struct tcp_socket_t* socket, struct tcp_header_t* request, struct handler_t* handler) {
    
    struct transmission_control_block_t* tcb = 
        (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", 
            sizeof(struct transmission_control_block_t));

    tcb->id = connection_id;    

    tcb->socket = socket;
    
    tcb->send_initial_sequence_num = tcp_generate_sequence_number();
    tcb->send_next = tcb->send_initial_sequence_num;
    tcb->send_window = 512; // hard-coded for now 

    tcb->send_unacknowledged = 0;
    tcb->send_urgent_pointer = 0;
    tcb->send_last_update_sequence_num = 0;
    tcb->send_last_update_acknowledgement_num = 0;        
    
    tcb->receive_initial_sequence_num = request->sequence_num;
    tcb->receive_window = request->window;
    tcb->receive_urgent_pointer = request->urgent_pointer;
    tcb->receive_next = tcb->receive_initial_sequence_num + 1;

    tcb->state = LISTEN; 
    
    tcb->state_function = tcp_listen;

    return tcb;
}

struct tcp_socket_t* tcp_get_socket() {
    return 0;
}

uint16_t tcp_handle_pre_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface) { 
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx - 1];
    struct tcp_header_t* tcp_request_header = (struct tcp_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct tcp_header_t* tcp_response_buffer = (struct tcp_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, tcp_request_header);    
    struct transmission_control_block_t* tcb = get_transmission_control_block(connection_id);

    uint16_t num_bytes_written = tcb->state_function(tcp_request_header, tcp_response_buffer, tcb);
    response_buffer->offset = num_bytes_written;

    return num_bytes_written;
}

uint16_t tcp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {   
    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct tcp_header_t* header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_idx];
    
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_idx - 1];
    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, header);
    
    struct transmission_control_block_t* tcb = get_transmission_control_block(connection_id);

    packet_stack->pre_build_response[packet_idx] = tcp_handle_pre_response;
                    
    if(tcb) {        
        handler_response(packet_stack, interface);                         
    } else if(header->control_bits & TCP_SYN_FLAG) {
        struct tcp_socket_t* socket = tcp_get_socket(ipv4_header->destination_ip, header->destination_port);

        if(socket) {
            create_transmission_control_block(connection_id, socket, header, handler);
            handler_response(packet_stack, interface);                         
        }        
    }

    NETSTACK_LOG(NETSTACK_INFO, "Received unexpected TCP packet: Dropping packet.\n");          
    return 1;
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