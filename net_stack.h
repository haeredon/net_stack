#ifndef NETSTACK_H
#define NETSTACK_H

#include "handlers/handler.h"
#include "handlers/interface.h"

enum net_stack_state_t {
    NET_STACK_STOPPED,
    NET_STACK_RUNNING
};

struct net_stack_app {
    struct handler_t** root_handlers;

	struct handler_t* arp_handler;
	struct handler_t* ipv4_handler;
	struct handler_t* tcp_handler;
	struct handler_t* ethernet_handler;

    struct interface_t* interface;   
    
    enum net_stack_state_t state;  
};


struct net_stack_app* net_stack_init(struct interface_t* interface);


#endif // NETSTACK_H