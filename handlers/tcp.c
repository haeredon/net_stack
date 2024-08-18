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
 * 
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

uint32_t tcp_generate_sequence_number() {
    return 42;
}

uint32_t tcb_header_to_connection_id(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    uint64_t id = ((uint64_t) ipv4_header->source_ip) << 32;
    id |= ((uint32_t) tcp_header->source_port) << 16;
    id |= tcp_header->destination_port;
    return id;
}

struct transmission_control_block_t* get_transmission_control_block(struct tcp_header_t* header) {
    uint32_t id = tcb_header_to_id(header);

    for (uint8_t i = 0 ; i < TRANSMISSION_CONTROL_BLOCK_BUFFER_SIZE ; i++) {
        struct transmission_control_block_t* tcb = transmission_blocks[i];
        
        if(tcb) {
            
        } else {
            return 0; // end of array
        }
    }   
    return 0; 
}

uint16_t tcp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {   
    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct tcp_header_t* header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_idx];
    
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_idx - 1];
    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, header);
    
    struct transmission_control_block_t* tcb = get_transmission_control_block(header);

    if(tcb) {
        
    } else if(header->control_bits & TCP_SYN_FLAG) {
        struct transmission_control_block_t* tcb = 
            (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", sizeof(struct transmission_control_block_t));
        
        tcb->send_initial_sequence_num = tcp_generate_sequence_number();
        tcb->send_next = tcb->send_initial_sequence_num;
        tcb->send_window = 512; // hard-coded for now 

        tcb->send_unacknowledged = 0;
        tcb->send_urgent_pointer = 0;
        tcb->send_last_update_sequence_num = 0;
        tcb->send_last_update_acknowledgement_num = 0;        
        
        tcb->receive_initial_sequence_num = header->sequence_num;
        tcb->receive_window = header->window;
        tcb->receive_urgent_pointer = header->urgent_pointer;
        tcb->receive_next = tcb->receive_initial_sequence_num + 1;

        tcb->state = SYN_RECEIVED;        
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