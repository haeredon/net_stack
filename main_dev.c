#include "handlers/handler.h"
#include "handlers/socket.h"
#include "worker.h"
#include "execution_pool.h"

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


    //////////////////////// HANDLER STUFF ///////////////////////////////////

	// set handlers and choose memory allocator
	struct handler_config_t *handler_config = (struct handler_config_t*) rte_zmalloc("Handler Config", sizeof(struct handler_config_t), 0);	
	handler_config->mem_allocate = net_stack_malloc;
	handler_config->mem_free = rte_free;
		
	struct handler_t** handlers = handler_create_stacks(handler_config);

    //////////////////////// THREADING STUFF ////////////// 
    struct execution_pool_t execution_pool = create_execution_pool(net_stack_malloc, 3);
    execution_pool.open(&execution_pool);


    /*
    * // inside handles, get the exection_contexts like this
    * struct execution_context_t execution_context = execution_pool.get_execution_context(hash);
    * execution_context.enqueue(package);
    * 
    * // when doing package read, do it like this
    * struct execution_context_t execution_context = execution_pool.get_any_execution_context();
    * execution_context.enqueue(package);
    * 
    */

    ////////////////////// NETWORK LAYER STUFF ///////////////////////////////////////////////////////
    // NetworkOffloader offloader = create_network_offloader();
    
    //////////////////////// USER STUFF /////////////////////////////////////////////
    void receive_callback() {
       
    }
    
    void open_callback() {
       socket.send(receive_callback); 
    }
  
    void create_callback() {
       socket.open(open_callback); 
    }

    Socket socket = create_tcp_socket(create_callback); // this function enques the creation of the socket on the thread queue

    while(1);
    
    return 0;
}