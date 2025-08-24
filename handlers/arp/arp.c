#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "handlers/arp/arp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/handler.h"

#include "util/array.h"

#include "util/log.h"

/*
 * Address Resolution Protocol handler. Only supports IPv4 over ethernet.
 * Specification: RFC 826
 */


// very basic slow implementation of ARP mapping. Should be optimized when more is known about use cases
struct arp_entry_t {
    uint32_t ipv4;
    uint8_t mac[ETHERNET_MAC_SIZE];
};

struct arp_resoltion_list_t {
    struct arp_entry_t** list;
    uint16_t insert_idx;
};

const uint16_t ARP_RESOLUTION_LIST_SIZE = 256;
struct arp_resoltion_list_t arp_resolution_list; 

void arp_insert_mapping(struct arp_entry_t* entry) {
    struct arp_entry_t* old_entry = arp_resolution_list.list[arp_resolution_list.insert_idx];
    if(old_entry) {
        free(old_entry);
    }
    
    arp_resolution_list.list[arp_resolution_list.insert_idx] = entry;
    arp_resolution_list.insert_idx = ++arp_resolution_list.insert_idx % ARP_RESOLUTION_LIST_SIZE;
}

struct arp_entry_t* arp_get_ip_mapping(uint32_t ipv4) {
    for (uint32_t i = 0; i < ARP_RESOLUTION_LIST_SIZE; i++) {
        struct arp_entry_t* entry = arp_resolution_list.list[i];

        if(!entry) {
            break;
        }

        if(ipv4 == entry->ipv4) {
            return entry;
        }        
    }

    return 0;    
}

void arp_close_handler(struct handler_t* handler) {
    struct arp_priv_t* private = (struct arp_priv_t*) handler->priv;    
    handler->handler_config->mem_free(private);
}

void arp_init_handler(struct handler_t* handler, void* priv_config) {
    struct arp_priv_t* arp_priv = (struct arp_priv_t*) handler->handler_config->mem_allocate("pcap handler private data", sizeof(struct arp_priv_t)); 
    handler->priv = (void*) arp_priv;

    arp_resolution_list.list = (struct arp_entry_t**) handler->handler_config->mem_allocate("Arp resolution list", sizeof(struct arp_entry_t) * ARP_RESOLUTION_LIST_SIZE);     

    for (uint32_t i = 0; i < ARP_RESOLUTION_LIST_SIZE; i++) {
        arp_resolution_list.list[i] = 0;
    }
}


bool arp_write(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler) {
    struct arp_write_args_t* arp_args = (struct arp_write_args_t*) packet_stack->args[packet_stack->stack_idx];    

    struct out_buffer_t* out_buffer = &packet_stack->out_buffer;
    struct arp_header_t* response_header = (struct arp_header_t*) ((uint8_t*) out_buffer->buffer + out_buffer->offset - sizeof(struct arp_header_t));

    if(out_buffer->offset < sizeof(struct arp_header_t)) {
        NETSTACK_LOG(NETSTACK_ERROR, "No room for arp header in buffer\n");         
        return false;   
    };

    memcpy(response_header, &arp_args->header, sizeof(struct arp_header_t));
  
    out_buffer->offset -= sizeof(struct arp_header_t);
   
    // if this is the bottom of the packet stack, then write to the interface
    if(!packet_stack->stack_idx) {
        handler->handler_config->write(out_buffer, interface, 0);
    } else {
        const struct handler_t* next_handler = packet_stack->handlers[--packet_stack->stack_idx];
        return next_handler->operations.write(packet_stack, interface, next_handler);        
    }
    
    return true;
}   

uint16_t arp_read(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler) {
    uint8_t packet_idx = packet_stack->stack_idx;
    packet_stack->handlers[packet_idx] = handler;  

    struct arp_header_t* header = (struct arp_header_t*) packet_stack->in_buffer.packet_pointers[packet_idx];
 
    if(header->hdw_type == ARP_HDW_TYPE_ETHERNET) {
        if(header->pro_type == ETHERNET_TYPE_IPV6) {
            struct arp_entry_t* mapping = 0;

            if(mapping = arp_get_ip_mapping(header->sender_protocol_addr)) {
                memcpy(mapping->mac, header->sender_hardware_addr, ETHERNET_MAC_SIZE);                
                mapping->ipv4 = header->sender_protocol_addr;
            }

            if(header->target_protocol_addr == interface->ipv4_addr) {
                if(!mapping) {
                    struct arp_entry_t* new_arp_entry = handler->handler_config->mem_allocate("arp entry", sizeof(struct arp_entry_t));
                    new_arp_entry->ipv4 = header->sender_protocol_addr;
                    memcpy(new_arp_entry->mac, header->sender_hardware_addr, ETHERNET_MAC_SIZE);
                    arp_insert_mapping(new_arp_entry);
                }

                if(header->operation == ARP_OPERATION_REQUEST) {
                    struct arp_write_args_t arp_write_args = {
                        .header = {
                            .hdw_type = header->hdw_type,
                            .pro_type = header->pro_type,
                            .hdw_addr_length = header->hdw_addr_length,
                            .pro_addr_length = header->pro_addr_length,
                            .operation = ARP_OPERATION_RESPOENSE,
                            .sender_hardware_addr = 0,
                            .sender_protocol_addr = header->target_protocol_addr,
                            .target_hardware_addr = 0,
                            .target_protocol_addr = header->sender_protocol_addr
                        }
                    };

                    memcpy(arp_write_args.header.sender_hardware_addr, interface->mac, ETHERNET_MAC_SIZE);                    
                    memcpy(arp_write_args.header.target_hardware_addr, header->sender_hardware_addr, ETHERNET_MAC_SIZE);

                    packet_stack->return_args[packet_idx] = &arp_write_args;

                    struct out_packet_stack_t* out_package_stack = handler_create_out_package_stack(packet_stack, packet_idx);

                    if(!handler->operations.write(0, interface, handler)) {
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}


struct handler_t* arp_create_handler(struct handler_config_t *handler_config) {
    struct handler_t* handler = (struct handler_t*) handler_config->mem_allocate("arp handler", sizeof(struct handler_t));	
    handler->handler_config = handler_config;

    handler->init = arp_init_handler;
    handler->close = arp_close_handler;

    handler->operations.read = arp_read;
    handler->operations.write = arp_write;

    ADD_TO_PRIORITY(&ethernet_type_to_handler, htons(ETHERNET_TYPE_ARP), handler);

    return handler;
}


