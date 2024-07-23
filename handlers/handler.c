#include "handlers/handler.h"

#include <stdint.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/pcapng.h"

// uint16_t kage(struct packet_stack_t* packet_stack, void* response_buffer) {
//     uint16_t num_written = 0;
//     for (uint8_t i = 0; i < packet_stack->stack_depth; i++) {
//         num_written += packet_stack->response[i](packet_stack->packet_pointers[i], response_buffer);        
//     }

//     return num_written;
// }


uint16_t handler_response(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv) {

}

// void handler_response(struct packet_stack_t* packet_stack, void* response_buffer) {
    // for (uint8_t i = 0; i < packet_stack->stack_depth; i++) {
    //     packet_stack->response[i](packet_stack->packet_pointers[i], response_buffer);        
    // }
// }


struct handler_t** handler_create_stacks(struct handler_config_t *config) {
	// these are root handlers which will be activated when a package arrives
	struct handler_t** handlers = (struct handler_t**) config->mem_allocate("handler array for ethernet", sizeof(struct handler_t*) * 2);
	
	handlers[0] = pcapng_create_handler(config);
	handlers[1] = ethernet_create_handler(config);
	
	// these are handlers which will not be activated when a package arrives, but must still be 
	// created because they might be called by a root handler
    struct handler_t* arp_handler = arp_create_handler(config); // TODO: fix memory leak

    return handlers;
}