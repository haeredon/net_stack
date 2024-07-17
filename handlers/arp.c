#include <arpa/inet.h>

#include <rte_malloc.h>
#include <rte_mbuf.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/handler.h"



uint16_t kage(struct packet_stack_t* packet_stack, void* response_buffer) {
    uint16_t num_written = 0;
    for (uint8_t i = 0; i < packet_stack->stack_depth; i++) {
        num_written += packet_stack->response[i](packet_stack->packet_pointers[i], response_buffer);        
    }

    return num_written;
}


void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    rte_free(private);
}

void arp_init_handler(struct handler_t* handler) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) rte_zmalloc("pcap handler private data", sizeof(struct arp_priv_t), 0); 
    handler->priv = (void*) arp_priv;
}

uint16_t arp_read(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    RTE_LOG(INFO, USER1, "ARP read handler called.\n");   

    uint8_t packet_idx = packet_stack->stack_depth;
    struct arp_header_t* header = (struct arp_header_t*) packet_stack->packet_pointers[packet_idx];
    uint16_t written = 0;

    // is it a request?
    if(!header->target_hardware_addr) {
        void* response = get_response_buffer(); // make the dpdk circular buffer available to this call               
        
        if(kage(packet_stack, response)) { // call response chain      
            RTE_LOG(ERR, USER1, "Could not create ARP packet response.\n");       
        } else {
            write(packet); // finally write the response
        }
    } 
    // is it a gratuitous ARP?
    else if(
        // method 1
        (header->target_protocol_addr == header->sender_protocol_addr && !header->target_hardware_addr) ||
        // method 2
        (header->target_protocol_addr == header->sender_protocol_addr && 
        header->target_hardware_addr == header->sender_hardware_addr)) {
                        
    } else {
        RTE_LOG(INFO, USER1, "Received invalid ARP packet.\n");   
    }

        
}

struct handler_t* arp_create_handler(void* (*mem_allocate)(const char *type, size_t size, unsigned align)) {
    struct handler_t* handler = (struct handler_t*) mem_allocate("arp handler", sizeof(struct handler_t), 0);	

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(0x0806), handler);

    return handler;
}


/******************* Initialization ************************/
// 1. register response handlers for all handlers
    // maybe use priority queue for this?

/***************** Handling (writing) *******************/
// 1. get a buffer to write to         
// 2. pass the response buffer to the response handler stack


/****************** Handling (not writing) *****************************/
// 1. add own reponse handler to packet_stack object
// 2. increment write_chain_lenght

// remember that the structure packet_stack_t at the beginning of read function always must have 
// write_chain_length set to the index the packet_pointer which the handler should work on.
// before leaving the read function write_chain_length must be incremented and the index in 
// must be set to point to the next data in the incoming buffer 