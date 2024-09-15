#include "tcp.h"
#include "handlers/ipv4/ipv4.h"
#include "log.h"
#include "tcp_out_buffer.h"
#include "tcp_block_buffer.h"


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

#define SOCKET_BUFFER_SIZE 64
struct tcp_socket_t* tcp_sockets[SOCKET_BUFFER_SIZE];

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
    memset(tcp_sockets, 0, sizeof(tcp_sockets));
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

uint16_t _tcp_calculate_checksum(struct tcp_header_t* tcp_header, struct transmission_control_block_t* tcb)  {
        struct tcp_pseudo_header_t pseudo_header = {
            .source_ip = tcb->own_ipv4,
            .destination_ip = tcb->remote_ipv4,
            .zero = 0,
            .ptcl = 6,
            .tcp_length = 0
        };

        return tcp_calculate_checksum(&pseudo_header, tcp_header);
}    

uint16_t tcp_get_payload_length(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header) {
    // it is multiplied by 4 because the header sizes are calculated in 32 bit chunks
    return ipv4_header->total_length - 
        ((tcp_header->data_offset & TCP_DATA_OFFSET_MASK) + (ipv4_header->flags_1 & IPV4_IHL_MASK)) * 4;
}

void add_to_in_buffer(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header, void* data, uint32_t payload_length) {
    uint32_t buffer_offset = (tcb->receive_initial_sequence_num % tcp_header->sequence_num);
    memcpy (((uint8_t*) tcb->in_buffer) + buffer_offset, data, payload_length);

    tcb->receive_window = TCP_RECEIVE_WINDOW - buffer_offset;
}

bool is_acknowledgement_valid(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header) {
    return tcb->send_unacknowledged < tcp_header->acknowledgement_num && 
       tcb->send_next >= tcp_header->acknowledgement_num;
}

bool is_segment_in_window(struct transmission_control_block_t* tcb, struct tcp_header_t* tcp_header, uint32_t payload_size) {
    if(payload_size) {
        uint32_t sequence_end = tcp_header->sequence_num + payload_size - 1 /* -1 because it says in RFC, maybe think it through */;
        return tcb->receive_window && (tcp_header->sequence_num >= tcb->receive_next &&
               tcp_header->sequence_num < tcb->receive_next + tcb->receive_window && 
               sequence_end < tcb->receive_next + tcb->receive_window);
    } else {
        return ((tcb->receive_window) && (tcp_header->sequence_num >= tcb->receive_next &&
               tcp_header->sequence_num < tcb->receive_next + tcb->receive_window)) || 
               (!tcb->receive_window && tcb->receive_next == tcp_header->sequence_num);
    }
}

uint16_t tcp_established(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header, 
                         struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {

    uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);

    if(is_segment_in_window(tcb, tcp_header, payload_size)) {

        if(ACK is set) {
            if(is_acknowledgement_valid(tcb, tcp_header)) {
                tcb->send_unacknowledged = tcp_header->acknowledgement_num;

                // send to input queue
                // flush input queue to user if needed
                // send acknowledgement
            } 

            // if it is not an old packet, then update the window
            if(tcb->send_last_update_sequence_num < tcp_header->sequence_num ||
               (tcb->send_last_update_sequence_num == tcp_header->sequence_num && 
                tcb->send_last_update_acknowledgement_num <= tcp_header->acknowledgement_num)
            ) {
                tcb->send_window = tcp_header->window;

                tcb->send_last_update_sequence_num = tcp_header->sequence_num;
                tcb->send_last_update_acknowledgement_num = tcp_header->acknowledgement_num;            
            }

            if(FIN is set) {
                tcb->state = CLOSE_WAIT;
            }
        } else {
            // drop segment and return
        }

    } else {
        // send <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
        // drop packet and return
    }
}

uint16_t tcp_syn_received(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header, 
                          struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {
    uint16_t payload_size = tcp_get_payload_length(ipv4_header, tcp_header);
    
    if((tcp_header->control_bits | TCP_ACK_FLAG == TCP_ACK_FLAG) && 
       is_segment_in_window(tcb, tcp_header, payload_size)) {

        if(!is_acknowledgement_valid(tcb, tcp_header)) {
            // clean up
            // send reset <SEQ=SEG.ACK><CTL=RST>
        }

        if(payload_size) {
            void* payload_start = ((uint8_t*) tcp_header) + tcp_header->data_offset;
            add_to_in_buffer(tcb, tcp_header, payload_start, payload_size);            
        }

        tcb->receive_next = tcb->receive_next + tcp_get_payload_length(ipv4_header, tcp_header);
        tcb->receive_urgent_pointer = 0;

        tcb->send_window = tcp_header->window;
        tcb->send_last_update_sequence_num = tcp_header->sequence_num;
        tcb->send_last_update_acknowledgement_num = tcp_header->acknowledgement_num;

        tcb->state = ESTABLISHED;
        tcb->state_function = tcp_established;

        response_buffer->source_port = tcp_header->destination_port;
        response_buffer->destination_port = tcp_header->source_port;
        response_buffer->sequence_num = tcb->send_next;
        response_buffer->acknowledgement_num = tcb->receive_next;
        response_buffer->data_offset = sizeof(struct tcp_header_t) / 32; // data offset is counted in 32 bit chunks 
        response_buffer->control_bits = TCP_ACK_FLAG;
        response_buffer->window = tcb->send_window;
        response_buffer->checksum = _tcp_calculate_checksum(tcp_header, tcb);
        response_buffer->urgent_pointer = 0; // Not supported

        tcb->send_next++;

        // send response
    } else {
        // clean up and connection reset 
        NETSTACK_LOG(NETSTACK_INFO, "TCP unexpected data: Dropping packet.\n");                 
    }
    
    return sizeof(struct tcp_header_t);
}

uint16_t tcp_listen(struct ipv4_header_t* ipv4_header, struct tcp_header_t* tcp_header, 
                    struct tcp_header_t* response_buffer, struct transmission_control_block_t* tcb) {
    tcb->receive_initial_sequence_num = tcp_header->sequence_num;
    tcb->receive_next = tcb->receive_initial_sequence_num + 1;
    tcb->receive_window = tcp_header->window;
    tcb->receive_urgent_pointer = 0;

    tcb->state = SYN_RECEIVED;
    tcb->state_function = tcp_syn_received;
    
    response_buffer->source_port = tcp_header->destination_port;
    response_buffer->destination_port = tcp_header->source_port;
    response_buffer->sequence_num = tcb->send_next;
    response_buffer->acknowledgement_num = tcb->receive_next;
    response_buffer->data_offset = sizeof(struct tcp_header_t) / 32; // data offset is counted in 32 bit chunks 
    response_buffer->control_bits = TCP_ACK_FLAG | TCP_SYN_FLAG;
    response_buffer->window = tcb->send_window;
    response_buffer->checksum = _tcp_calculate_checksum(tcp_header, tcb);
    response_buffer->urgent_pointer = 0; // Not supported

    tcb->send_next++;
    tcb->send_unacknowledged = tcb->receive_initial_sequence_num;

    // send response
}

uint32_t tcp_generate_sequence_number() {
    return 42;
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

uint16_t create_transmission_control_block(uint32_t connection_id, 
    struct tcp_socket_t* socket, struct tcp_header_t* tcp_request, 
    struct ipv4_header_t* ipv4_request, struct handler_t* handler) {
    
    struct transmission_control_block_t* tcb = 
        (struct transmission_control_block_t*) handler->handler_config->mem_allocate("tcp: new session", 
            sizeof(struct transmission_control_block_t));

    tcb->id = connection_id;    

    tcb->socket = socket;

    tcb->own_ipv4 = ipv4_request->destination_ip;
    tcb->remote_ipv4 = ipv4_request->source_ip;
    
    tcb->send_initial_sequence_num = tcp_generate_sequence_number();
    tcb->send_next = tcb->send_initial_sequence_num;
    tcb->send_window = 512; // hard-coded for now 

    tcb->send_unacknowledged = 0;
    tcb->send_urgent_pointer = 0;
    tcb->send_last_update_sequence_num = 0;
    tcb->send_last_update_acknowledgement_num = 0;        
    
    tcb->receive_initial_sequence_num = tcp_request->sequence_num;
    tcb->receive_window = TCP_RECEIVE_WINDOW;
    tcb->receive_urgent_pointer = tcp_request->urgent_pointer;
    tcb->receive_next = tcb->receive_initial_sequence_num + 1;

    tcb->in_buffer = create_tcp_block_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);
    tcb->out_buffer = create_tcp_out_buffer(10, handler->handler_config->mem_allocate, handler->handler_config->mem_free);

    tcb->state = LISTEN; 
    
    tcb->state_function = tcp_listen;

    return tcb;
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

bool tcp_add_transmission_control_block(struct transmission_control_block_t* tcb) {
    for (uint64_t i = 0; i < TRANSMISSION_CONTROL_BLOCK_BUFFER_SIZE; i++) {
        if(!transmission_blocks[i]) {
            transmission_blocks[i] = tcb;
            return true;
        }        
    }
    return false;;          
}


struct tcp_socket_t* tcp_get_socket(uint16_t port, uint32_t ipv4) {
    for (uint8_t i = 0 ; i < SOCKET_BUFFER_SIZE && tcp_sockets[i] ; i++) {
        struct tcp_socket_t* socket = tcp_sockets[i];
        
        if(socket->listening_port == port && socket->interface->ipv4_addr == ipv4) {
            return socket;
        } 
    }   
    return 0; 
}

bool tcp_add_socket(struct tcp_socket_t* socket, struct handler_t* handler) {
    for (uint64_t i = 0; i < SOCKET_BUFFER_SIZE; i++) {
        if(!tcp_sockets[i]) {
            tcp_sockets[i] = socket;
            return true;
        }        
    }
    return false;     
}


uint16_t tcp_handle_pre_response(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface) { 
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx - 1];
    struct tcp_header_t* tcp_request_header = (struct tcp_header_t*) packet_stack->packet_pointers[response_buffer->stack_idx];
    struct tcp_header_t* tcp_response_buffer = (struct tcp_header_t*) (((uint8_t*) (response_buffer->buffer)) + response_buffer->offset);

    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, tcp_request_header);    
    struct transmission_control_block_t* tcb = get_transmission_control_block(connection_id);

    uint16_t num_bytes_written = tcb->state_function(ipv4_header, tcp_request_header, tcp_response_buffer, tcb);;    
    response_buffer->offset = num_bytes_written;

    return num_bytes_written;
}


uint16_t tcp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {   
    uint8_t packet_idx = packet_stack->write_chain_length++;
    struct tcp_header_t* header = (struct tcp_header_t*) packet_stack->packet_pointers[packet_idx];
    
    struct ipv4_header_t* ipv4_header = (struct ipv4_header_t*) packet_stack->packet_pointers[packet_idx - 1];
    uint32_t connection_id = tcb_header_to_connection_id(ipv4_header, header);
    
    struct tcp_socket_t* socket = tcp_get_socket(header->destination_port, ipv4_header->destination_ip);

    if(socket) {
        struct transmission_control_block_t* tcb = get_transmission_control_block(connection_id);

        if(!tcb) {
            tcb = create_transmission_control_block(connection_id, socket, header, ipv4_header, handler);
            
            if(!tcp_add_transmission_control_block(tcb)) {
                handler->handler_config->mem_free(tcb);
                NETSTACK_LOG(NETSTACK_INFO, "Could not allocate transmission control block.\n");          
                return 1;
            } 
        } 

        uint16_t payload_size = tcp_get_payload_length(ipv4_header, header);
        tcp_block_buffer_add(tcb->in_buffer, header->sequence_num, header, payload_size);
        
        uint16_t num_ready = tcp_block_buffer_num_ready(tcb->in_buffer);
        
        if(num_ready) {
            tcb->state_function(tcb, num_ready);
        }
    }
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