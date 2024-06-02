#include "handlers/handler.h"

#include <stdint.h>
#include <rte_malloc.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/pcapng.h"

struct handler_t** handler_create_stacks(void* (*mem_allocate)(const char *type, size_t size, unsigned align)) {
	struct handler_t** handlers = (struct handler_t**) mem_allocate("handler array for ethernet", sizeof(struct handler_t*) * 2, 0);
	handlers[0] = pcapng_create_handler(mem_allocate);
	handlers[1] = ethernet_create_handler(mem_allocate);
	
    struct handler_t* arp_handler = arp_create_handler(mem_allocate);

    return handlers;
}