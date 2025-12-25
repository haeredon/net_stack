#include "handlers/ethernet/ethernet.h"
#include "handlers/arp/arp.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp.h"
#include "handlers/handler.h"
#include "util/memory.h"

#include "net_stack.h"


struct net_stack_app* net_stack_init(struct interface_t* interface) {    
	// create config which contains write information for all handlers
	struct handler_config_t *handler_config = (struct handler_config_t*) NET_STACK_MALLOC("Handler Config", sizeof(struct handler_config_t));	
	handler_config->write = handler_write;

	// these are root handlers which will be activated when a package arrives
	struct handler_t** handlers = (struct handler_t**) NET_STACK_MALLOC("handler array for ethernet", sizeof(struct handler_t*));
	
	handlers[0] = ethernet_create_handler(handler_config);
	
	// these are handlers which will not be activated when a package arrives, but must still be 
	// created because they might be called by a root handler
    struct handler_t* arp_handler = arp_create_handler(handler_config); // TODO: fix memory leak
	arp_handler->init(arp_handler, 0);

	struct handler_t* ipv4_handler = ipv4_create_handler(handler_config); // TODO: fix memory leak
	ipv4_handler->init(ipv4_handler, 0);

    struct tcp_priv_config_t tcp_config = {
        .window = 4096
    };
    struct handler_t* tcp_handler = tcp_create_handler(handler_config); // TODO: fix memory leak
	tcp_handler->init(tcp_handler, (void*) &tcp_config);

    struct net_stack_app* app = (struct net_stack_app*) NET_STACK_MALLOC("net stack app", sizeof(struct net_stack_app));

    app->arp_handler = arp_handler;
    app->ipv4_handler = ipv4_handler;
    app->tcp_handler = tcp_handler;
    app->ethernet_handler = handlers[0];

    app->root_handlers = handlers;

    app->interface = interface;

    return app;
}