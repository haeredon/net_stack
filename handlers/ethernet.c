#include "handlers/ethernet.h"
#include "handlers/protocol_map.h"

#include <rte_malloc.h>




struct key_val_t key_vals[ETHERNET_NUM_ETH_TYPE_ENTRIES];
struct priority_map_t ethernet_type_to_handler = { 
    .max_size = ETHERNET_NUM_ETH_TYPE_ENTRIES,
    .map = key_vals
};


void ethernet_init_handler(struct handler_t* handler) {
    struct ethernet_priv_t* ethernet_priv = (struct ethernet_priv_t*) rte_zmalloc("pcap handler private data", sizeof(struct ethernet_priv_t), 0); 
    handler->priv = (void*) ethernet_priv;
}

void ethernet_close_handler(struct handler_t* handler) {
    struct ethernet_priv_t* private = (struct ethernet_priv_t*) handler->priv;    
    rte_free(private);
}

uint16_t ethernet_response(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {
    RTE_LOG(WARNING, USER1, "ethernet_response() called");            
}

uint16_t ethernet_read(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {   
    uint8_t packet_idx = packet_stack->write_chain_length;
    struct ethernet_header_t* header = (struct ethernet_header_t*) packet_stack->packet_pointers[packet_idx];

    packet_stack->response[packet_idx] = ethernet_response;

    if(header->ethernet_type > 0x0600) {
        struct handler_t* handler = GET_FROM_PRIORITY(
            &ethernet_type_to_handler,
            header->ethernet_type, 
            struct handler_t
        );

        if(handler) {
            // set next buffer pointer for next protocol level     
            packet_stack->packet_pointers[++packet_stack->write_chain_length] = ((uint64_t*) header) + sizeof(struct ethernet_header_t);
            
            handler->operations.read(packet_stack, interface, priv);            
        } else {
            RTE_LOG(WARNING, USER1, "Received non-supported ether_type: %hx\n", header->ethernet_type);            
        }        
    } else {
        RTE_LOG(WARNING, USER1, "Received IEEE 802.3 ethenet packet. This standard is not supported. ether_type: %hx\n", header->ethernet_type);        
    }    
}

void ethernet_init_header(void* buffer, uint8_t* source, uint8_t* dest, uint8_t* tag, uint8_t* type) {
    uint16_t offset = 0;
    
    memcpy(buffer, dest, ETHERNET_MAC_SIZE);
    offset += ETHERNET_MAC_SIZE;

    memcpy(buffer, source, ETHERNET_MAC_SIZE);
    offset += ETHERNET_MAC_SIZE;

    if(tag) {
        memcpy(buffer, dest, ETHERNET_TAG_SIZE);
        offset += ETHERNET_TAG_SIZE;
    }

    memcpy(buffer, type, ETHERNET_ETH_TYPE_SIZE);
    offset += ETHERNET_ETH_TYPE_SIZE;

    return offset;
}


struct handler_t* ethernet_create_handler(void* (*mem_allocate)(const char *type, size_t size, unsigned align)) {
    struct handler_t* handler = (struct handler_t*) mem_allocate("ethernet handler", sizeof(struct handler_t), 0);	

    handler->init = ethernet_init_handler;
    handler->close = ethernet_close_handler;

    handler->operations.read = ethernet_read;
    handler->operations.response = ethernet_response;

    return handler;
}