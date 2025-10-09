#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <fcntl.h>
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

#include "worker.h"
#include "handlers/handler.h"
#include "util/queue.h"
#include "util/log.h"
#include "util/memory.h"


int consume_tasks(void* execution_context_arg) {
    NETSTACK_LOG(NETSTACK_INFO, "Worker thread started\n");   

	struct execution_context_t* execution_context = (struct execution_context_t*) execution_context_arg;
	void* buffers[1]; // only support single buffer dequeue for now
     
	while(execution_context->state == NET_STACK_RUNNING) {
		int ret = QUEUE_DEQUEUE(execution_context->work_queue, buffers);	

		// if nothing was dequeued, just try again
		if(ret != 0) {
			continue;
		}

		NETSTACK_LOG(NETSTACK_DEBUG, "Dequed packet\n");   

		for(uint8_t i = 0 ; i < execution_context->num_handlers ; i++) {
			void* buffer = execution_context->get_packet_buffer(buffers[0]);
			struct interface_t* interface = execution_context->get_interface(buffers[0]);

			struct in_packet_stack_t packet_stack = { 
				.stack_idx = 0, 
				.in_buffer = { 
					.packet_pointers[0] = buffer
				} 
			};

			execution_context->handlers[i]->operations.read(&packet_stack, interface, execution_context->handlers[i]);

			execution_context->free_packet(buffer);
		}
	}	
}

void stop(struct execution_context_t* execution_context) {
	pthread_mutex_lock(&execution_context->state_lock);

	EXECUTION_CONTEXT_THREAD_STOP(execution_context->thread_handle);
	execution_context->state = NET_STACK_STOPPED;
	
	pthread_mutex_unlock(&execution_context->state_lock);
}

int start(struct execution_context_t* execution_context) {
	NETSTACK_LOG(NETSTACK_INFO, "Worker thread starting\n");   

	pthread_mutex_lock(&execution_context->state_lock);
	
	execution_context->state = NET_STACK_RUNNING;
    EXECUTION_CONTEXT_THREAD_CREATE(execution_context->thread_handle, consume_tasks, execution_context);

	pthread_mutex_unlock(&execution_context->state_lock);
}

struct execution_context_t* create_netstack_execution_context(uint8_t worker_id,
														      struct handler_t** handlers,
    														  uint8_t num_handlers, 
															  void* (*get_packet_buffer)(void* packet),
												   			  void (*free_packet)(void* packet),
															  struct interface_t* (*get_interface)(void* packet)) {
	struct execution_context_t* execution_context = (struct execution_context_t*) NET_STACK_MALLOC("Execution context", 
        sizeof(struct execution_context_t));

	execution_context->worker_id = worker_id;

	execution_context->start = start;
	execution_context->stop = stop;
	QUEUE_CREATE_QUEUE(execution_context->work_queue, QUEUE_SIZE);

	execution_context->get_packet_buffer = get_packet_buffer;
	execution_context->free_packet = free_packet;

	execution_context->handlers = handlers;
	execution_context->num_handlers = num_handlers;
	execution_context->get_interface = get_interface;

	pthread_mutex_init(&execution_context->state_lock, 0);    

	return execution_context;
}