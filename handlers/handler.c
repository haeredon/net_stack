#include "handlers/handler.h"

#include <stdint.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/pcapng.h"
#include "log.h"


uint16_t handler_response(struct packet_stack_t* packet_stack, struct interface_t* interface) {

	/*
	* Just some makeshift buffer for now
	*
	*/
	const uint16_t BUFFER_SIZE = 4096;
	uint8_t buffer[BUFFER_SIZE];
	struct response_buffer_t response_buffer = { .buffer = buffer, .offset = 0, .size = BUFFER_SIZE, .stack_idx = 0 };

	// call all response handlers to build packet
    for (uint8_t i = 0; i < packet_stack->write_chain_length; i++, response_buffer.stack_idx++) {
        packet_stack->response[i](packet_stack, &response_buffer, interface);        
    }

	// write buffer to interface
	int64_t ret = interface->operations.write(response_buffer.buffer, response_buffer.offset);
	if(ret < 0) {
		NETSTACK_LOG(NETSTACK_ERROR, "Failed to write packet");        
	}

	return ret;
}


struct handler_t** handler_create_stacks(struct handler_config_t *config) {
	// these are root handlers which will be activated when a package arrives
	struct handler_t** handlers = (struct handler_t**) config->mem_allocate("handler array for ethernet", sizeof(struct handler_t*) * 2);
	
	handlers[0] = pcapng_create_handler(config);
	handlers[1] = ethernet_create_handler(config);
	
	// these are handlers which will not be activated when a package arrives, but must still be 
	// created because they might be called by a root handler
    struct handler_t* arp_handler = arp_create_handler(config); // TODO: fix memory leak
	arp_handler->init(arp_handler);

    return handlers;
}