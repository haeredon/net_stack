#include "handlers/handler.h"
#include "handlers/socket.h"
#include "handlers/ethernet/ethernet.h"
#include "handlers/ipv4/ipv4.h"
#include "handlers/tcp/socket.h"
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

void receive_callback(uint8_t* data, uint64_t size) {
       // handle data
    }
    
void open_callback() {
    // get handler and socket information somehow
    //socket.send(socket, connection_id, handler); 
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
        
    // initialize socket
    struct tcp_socket_t* tcp_socket = tcp_create_socket(0, 1337, 0xAAAAAAAA, receive_callback); 

    tcp_socket.socket.handlers[0] = handlers[0]; // ethernet
    tcp_socket.socket.handlers[1] = handlers[1]; // ipv4
    tcp_socket.socket.handlers[2] = handlers[2]; // tcp

    struct ethernet_write_args_t eth_args = {
        .destination = { 0, 0, 0, 0, 0, 0},
        .ethernet_type = 0x0800
    };

    struct ipv4_write_args_t ipv4_args = {
        .destination_ip = 0xAAAAAAAA,
        .protocol = 0xFF       
    };

    tcp_socket.socket->handler_args[0] = &eth_args;
    tcp_socket.socket->handler_args[1] = &ipv4_args;
    // no need for tcp, because those args will be set by the open call to the tcp socket

    tcp_socket.socket.depth = 3;
    

    //ConnectionId connectionId = socket->open(socket, handler, remote_ip, port, open_callback):


    while(1);
    
    return 0;
}