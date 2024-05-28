#include "handlers/handler.h"

#include <stdint.h>
#include <rte_malloc.h>

#include "handlers/arp.h"
#include "handlers/ethernet.h"
#include "handlers/pcapng.h"

uint8_t handler_init(struct handler_t** handlers) {
	handlers = (struct handler_t**) rte_zmalloc("handler array for ethernet", sizeof(struct handler_t*) * 2, 0);
	handlers[0] = pcapng_create_handler();
	handlers[1] = ethernet_create_handler();
	
    struct handler_t* arp_handler = arp_create_handler();

    return 0;
}