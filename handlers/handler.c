#include "handlers/handler.h"

#include <stdint.h>

#include "handlers/arp/arp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/pcapng/pcapng.h"
#include "handlers/ipv4/ipv4.h"
#include "util/log.h"
#include "util/memory.h"


uint16_t handler_write(struct out_buffer_t* buffer, struct interface_t* interface, struct transmission_config_t* transmission_config) {
	interface->operations.write(buffer);
}

struct out_packet_stack_t* handler_create_out_package_stack(struct in_packet_stack_t* packet_stack, uint8_t package_depth) {
    struct out_packet_stack_t* out_package_stack = (struct out_packet_stack_t*) NET_STACK_MALLOC("response: out_package_stack", 
        DEFAULT_PACKAGE_BUFFER_SIZE + sizeof(struct out_packet_stack_t)); 

    memcpy(out_package_stack->handlers, packet_stack->handlers, 10 * sizeof(struct handler_t*));
    memcpy(out_package_stack->args, packet_stack->return_args, 10 * sizeof(void*));

    out_package_stack->out_buffer.buffer = (uint8_t*) out_package_stack + sizeof(struct out_packet_stack_t);
    out_package_stack->out_buffer.size = DEFAULT_PACKAGE_BUFFER_SIZE;
    out_package_stack->out_buffer.offset = DEFAULT_PACKAGE_BUFFER_SIZE;      

    out_package_stack->stack_idx = package_depth;

    return out_package_stack;
}


struct handler_t** handler_create_stacks(struct handler_config_t *config) {
	// these are root handlers which will be activated when a package arrives
	struct handler_t** handlers = (struct handler_t**) NET_STACK_MALLOC("handler array for ethernet", sizeof(struct handler_t*) * 2);
	
	// handlers[0] = pcapng_create_handler(config);
	handlers[0] = ethernet_create_handler(config);
	
	// these are handlers which will not be activated when a package arrives, but must still be 
	// created because they might be called by a root handler
    struct handler_t* arp_handler = arp_create_handler(config); // TODO: fix memory leak
	arp_handler->init(arp_handler, 0);

	struct handler_t* ipv4_handler = ipv4_create_handler(config); // TODO: fix memory leak
	ipv4_handler->init(ipv4_handler, 0);

    return handlers;
}