#include "handlers/ethernet.h"
#include "handlers/handler.h"

#include <rte_malloc.h>


struct key_val_t ethernet_type_to_handler[ETHERNET_NUM_ETH_TYPE_ENTRIES];

void ethernet_init_handler(struct handler_t* handler) {
    struct ethernet_priv_t* ethernet_priv = (struct ethernet_priv_t*) rte_zmalloc("pcap handler private data", sizeof(struct ethernet_priv_t), 0); 
    handler->priv = (void*) ethernet_priv;
}

void ethernet_close_handler(struct handler_t* handler) {
    struct ethernet_priv_t* private = (struct ethernet_priv_t*) handler->priv;    
    rte_free(private);
}

uint16_t ethernet_read(struct rte_mbuf* buffer, struct interface_t* interface, void* priv) {   

    struct ethernet_header_t* header = rte_pktmbuf_mtod(buffer, struct ethernet_header_t*);

    if(header->ethernet_type > 0x0600) {
        struct handler_t* handler = GET_FROM_PRIORITY(
            ethernet_type_to_handler,
            header->ethernet_type, 
            struct handler_t
        );

        if(handler) {
            handler->operations.read(buffer, interface, priv);            
        } else {
            RTE_LOG(WARNING, USER1, "Received non-supported ether_type: %hx\n", header->ethernet_type);            
        }        
    } else {
        RTE_LOG(WARNING, USER1, "Received IEEE 802.3 ethenet packet. This standard is not supported. ether_type: %hx\n", header->ethernet_type);        
    }
    
}

struct handler_t* ethernet_create_handler() {
    struct handler_t* handler = (struct handler_t*) rte_zmalloc("ethernet handler", sizeof(struct handler_t), 0);	

    handler->init = ethernet_init_handler;
    handler->close = ethernet_close_handler;

    handler->operations.read = ethernet_read;
}