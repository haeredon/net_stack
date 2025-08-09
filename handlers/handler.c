#include "handlers/handler.h"

#include <stdint.h>

#include "handlers/arp/arp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/pcapng/pcapng.h"
#include "handlers/ipv4/ipv4.h"
#include "util/log.h"


// uint16_t handler_response(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config) {

	// /*
	// * Just some makeshift buffer for now
	// *
	// */
	// const uint16_t BUFFER_SIZE = 4096;
	// uint8_t buffer[BUFFER_SIZE];
	// struct response_buffer_t response_buffer = { .buffer = buffer, .offset = 0, .size = BUFFER_SIZE, .stack_idx = 0 };
	// uint16_t offsets[11] = { 0 }; // must be more dynamic when packet_stack becomes more dynamic
	
	// // call all pre response handlers to build packet
    // for (uint8_t i = 0; i < packet_stack->write_chain_length; i++, response_buffer.stack_idx++) {
	// 	uint16_t (*pre_build_response)() = packet_stack->pre_build_response[i];
	// 	if(pre_build_response) {						
	// 		offsets[i + 1] = pre_build_response(packet_stack, &response_buffer, interface);			
	// 	}    	    	
    // }

	// // call all post response handlers to build packet
    // for (uint8_t i = 0; i < packet_stack->write_chain_length; i++, response_buffer.stack_idx++) {
	// 	void (*post_build_response)(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, 
    //                               const struct interface_t* interface, uint16_t offset) = packet_stack->post_build_response[i];
	// 	if(post_build_response) {
	// 		post_build_response(packet_stack, &response_buffer, interface, offsets[i]);        
	// 	}    	    	
    // }

	// // write buffer to interface
	// struct response_t response = {
	// 	.buffer = response_buffer.buffer,
	// 	.size = response_buffer.offset,
	// 	.interface = interface
	// };
	// int64_t ret = interface->operations.write(response);
	// if(ret < 0) {
	// 	NETSTACK_LOG(NETSTACK_ERROR, "Failed to write packet");        
	// }

	// return ret;
// }


struct handler_t** handler_create_stacks(struct handler_config_t *config) {
	// these are root handlers which will be activated when a package arrives
	struct handler_t** handlers = (struct handler_t**) config->mem_allocate("handler array for ethernet", sizeof(struct handler_t*) * 2);
	
	// handlers[0] = pcapng_create_handler(config);
	handlers[0] = ethernet_create_handler(config);
	
	// these are handlers which will not be activated when a package arrives, but must still be 
	// created because they might be called by a root handler
    // struct handler_t* arp_handler = arp_create_handler(config); // TODO: fix memory leak
	// arp_handler->init(arp_handler, 0);

	struct handler_t* ipv4_handler = ipv4_create_handler(config); // TODO: fix memory leak
	ipv4_handler->init(ipv4_handler, 0);

    return handlers;
}