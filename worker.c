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
#include "log.h"

#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

volatile bool force_quit = false; 

static void worker_main_loop(struct lcore_setup_t* setup)
{
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, portid, nb_rx;

	prev_tsc = 0;
	timer_tsc = 0;

	lcore_id = rte_lcore_id();
	struct interface_t* interface = &setup->interface;
	struct handler_t** handlers = setup->handlers;

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	while (!force_quit) {
		/* Read packet from RX queues. 8< */
		nb_rx = rte_eth_rx_burst(interface->port, 0,
						pkts_burst, MAX_PKT_BURST);

		if (unlikely(nb_rx == 0)) {
			continue;
		}			

		for (j = 0; j < nb_rx; j++) {
			struct rte_mbuf* buffer = pkts_burst[j];

			struct in_packet_stack_t packet_stack = { .stack_idx = 0, .in_buffer = { .packet_pointers = 0 } };

			void* buffer_start = rte_pktmbuf_mtod(buffer, void *);

			rte_prefetch0(buffer_start); // learn more about this statement!!!

			for (uint8_t i = 0; i < setup->num_handlers; i++) {
				handlers[i]->operations.read(&packet_stack, interface, handlers[i]);	
			}
		}

		/* >8 End of read packet from RX queues. */
	}
}

int worker_start_lcore_worker(void* setups) {
	unsigned lcore_id = rte_lcore_id();

	struct lcore_setup_t* lcore_setup = (struct lcore_setup_t*) setups;

	for (uint8_t i = 0; i < lcore_setup->num_handlers; i++) {
		struct handler_t* handler = lcore_setup->handlers[i];
		handler->init(handler, 0);
	}
	
	worker_main_loop(lcore_setup);
	
	for (uint8_t i = 0; i < lcore_setup->num_handlers; i++) {
		struct handler_t* handler = lcore_setup->handlers[i];
		handler->close(handler);
	}

	return 0;	
}

/*********************************************** */

void* consume_tasks(void* thread_arg) {
    NETSTACK_LOG(NETSTACK_INFO, "Worker thread started\n");   

	struct execution_context_t* execution_context = (struct execution_context_t*) thread_arg;
     
	while(true) {
		struct task_t task = queue_dequeue(execution_context->work_queue);	
		NETSTACK_LOG(NETSTACK_DEBUG, "Dequed task\n");   
		task->start();
	}
	
	
	return NULL;
}


int start(struct execution_context_t* thread) {
	NETSTACK_LOG(NETSTACK_INFO, "Worker thread starting\n");   

	pthread_t thread;
    return pthread_create(&thread, NULL, consume_tasks, thread);
}

struct execution_context_t create_netstack_thread(void* (*mem_allocate)(const char *type, size_t size)) {
	struct execution_context_t execution_context = {
		.start = start,
		.work_queue = queue_create_queue(mem_allocate)
	};

	return execution_context;
}