#include "handlers/handler.h"

#include <stdint.h>

#include "handlers/arp/arp.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/pcapng/pcapng.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/tcp.h"
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