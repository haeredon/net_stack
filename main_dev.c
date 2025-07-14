#include "handlers/handler.h"
#include "handlers/socket.h"
#include "worker.h"

#include <stdint.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_string_fns.h>
#include <rte_pcapng.h>


void* net_stack_malloc(const char *type, size_t size) {
	return rte_malloc(type, size, 0);
}


uint8_t main(int argc, char **argv) {


    // HANDLER STUFF ///////////////////////////////////

	// set handlers and choose memory allocator
	struct handler_config_t *handler_config = (struct handler_config_t*) rte_zmalloc("Handler Config", sizeof(struct handler_config_t), 0);	
	handler_config->mem_allocate = net_stack_malloc;
	handler_config->mem_free = rte_free;
		
	struct handler_t** handlers = handler_create_stacks(handler_config);

    // // initialize handler worker thread
    struct thread_t thread = create_netstack_thread();

    // thread.run();

    // thread.work_queue.enqueue(/* some package */);   

    // // NETWORK LAYER STUFF ///////////////////////////////////////////////////////
    // NetworkOffloader offloader = create_network_offloader();
    // offloader->affinity(some affinity stuff linking packages to threads through some filter function)

    // // CLIENT STUFF /////////////////////    

    // Socket socket = create_tcp_socket(); // this function enques the creation of the socket on the thread queue
    // socket.open();
    // socket.send(some package);






    while(1);
    
    return 0;
}