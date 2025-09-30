/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2016 Intel Corporation
 */

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
#include <rte_ring.h>

#include "util/memory.h"
#include "util/queue.h"
#include "worker.h"
#include "interface.h"
#include "handlers/handler.h"
#include "handlers/ethernet/ethernet.h"
#include "dpdk/write.h"
#include "dpdk/packet.h"

/* Ports set in promiscuous mode off by default. */
static int promiscuous_on = 1;

#define MAX_PKT_BURST 32
#define MEMPOOL_CACHE_SIZE 256

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RX_DESC_DEFAULT 1024
#define TX_DESC_DEFAULT 1024
static uint16_t num_rxd = RX_DESC_DEFAULT;
static uint16_t num_txd = TX_DESC_DEFAULT;

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

static struct rte_eth_conf port_conf = {
	.txmode = {
		.mq_mode = RTE_ETH_MQ_TX_NONE,
	},
};

struct rte_mempool * pktmbuf_pool = NULL;
uint64_t num_dropped_packages = 0;


/* Check the link status of all ports in up to 9s, and print them finally */
static void check_all_ports_link_status() {
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint16_t portid;
	uint8_t count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;
	int ret;
	char link_status_text[RTE_ETH_LINK_MAX_STR_LEN];

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		RTE_ETH_FOREACH_DEV(portid) {
			if (force_quit)
				return;
			memset(&link, 0, sizeof(link));
			ret = rte_eth_link_get_nowait(portid, &link);
			if (ret < 0) {
				all_ports_up = 0;
				if (print_flag == 1)
					printf("Port %u link get failed: %s\n",
						portid, rte_strerror(-ret));
				continue;
			}
			/* print link status if flag set */
			if (print_flag == 1) {
				rte_eth_link_to_str(link_status_text,
					sizeof(link_status_text), &link);
				printf("Port %d %s\n", portid,
				       link_status_text);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == RTE_ETH_LINK_DOWN && portid != 1) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

// does not work in current implementation
static void signal_handler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

bool force_quit = false; 

static void offloader_loop(struct interface_t* interface, 
						   struct execution_context_t** workers, uint8_t num_execution_contexts) {
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];

	while (!force_quit) {
		/* Read packet from RX queues. 8< */
		uint16_t nb_rx = rte_eth_rx_burst(interface->port, 0, pkts_burst, MAX_PKT_BURST);

		if (unlikely(nb_rx == 0)) {
			continue;
		}			

		for (uint16_t j = 0; j < nb_rx; j++) {
			struct rte_mbuf* buffer = pkts_burst[j];

			struct in_packet_stack_t packet_stack = { .stack_idx = 0, .in_buffer = { .packet_pointers = 0 } };

			for (uint8_t i = 0; i < num_execution_contexts; i++) {		
				rte_ring_sp_enqueue(workers[0]->work_queue, buffer);
			}
		}

		/* >8 End of read packet from RX queues. */
	}
}

int main(int argc, char **argv) {
	int ret;
	uint16_t portid;
	unsigned lcore_id;
	unsigned int num_lcores = 1; // hardcoded, should be detected dynmamically
	
	// init EAL
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	// Create the mbuf pool. 	
	unsigned int num_mbufs = RTE_MAX(2 /* num_ports */ * (num_rxd + num_txd + MAX_PKT_BURST +
		num_lcores * MEMPOOL_CACHE_SIZE), 8192U);

	pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", num_mbufs, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
		rte_socket_id());
	
	if (pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");


	// create net_stack interface
	struct interface_operations_t interface_operations = {
		.write = dpdk_write_write
	};
	struct interface_t* interface = interface_create_interface(interface_operations);
	bool first_port_initialized = false;	

	// Initialise each port 
	RTE_ETH_FOREACH_DEV(portid) {				
		struct rte_eth_conf local_port_conf = port_conf;
		struct rte_eth_dev_info dev_info;

		// init port
		printf("Initializing port %u... ", portid);
		fflush(stdout);

		ret = rte_eth_dev_info_get(portid, &dev_info);
		if (ret != 0)
			rte_exit(EXIT_FAILURE,
				"Error during getting device (port %u) info: %s\n",
				portid, strerror(-ret));

		if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
			local_port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
		
		// Configure the number of queues for a port.
		ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);		
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, portid);
		

		ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &num_rxd, &num_txd);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
				 "Cannot adjust number of descriptors: err=%d, port=%u\n",
				 ret, portid);

		// init one RX queue 
		fflush(stdout);
		struct rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
		rxq_conf.offloads = local_port_conf.rxmode.offloads;

		// RX queue setup. 
		ret = rte_eth_rx_queue_setup(portid, 0, num_rxd,
					     rte_eth_dev_socket_id(portid),
					     &rxq_conf,
					     pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, portid);
		
		// Init one TX queue on each port. 
		fflush(stdout);
		struct rte_eth_txconf txq_conf = dev_info.default_txconf;
		txq_conf.offloads = local_port_conf.txmode.offloads;
		ret = rte_eth_tx_queue_setup(portid, 0, num_txd,
				rte_eth_dev_socket_id(portid),
				&txq_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, portid);

		// Initialize TX buffers 
		tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				rte_eth_tx_buffer_count_callback,
				&num_dropped_packages);
		if (ret < 0)
			rte_exit(EXIT_FAILURE,
			"Cannot set error callback for tx buffer on port %u\n",
				 portid);

		ret = rte_eth_dev_set_ptypes(portid, RTE_PTYPE_UNKNOWN, NULL, 0);
		if (ret < 0)
			printf("Port %u, Failed to disable Ptype parsing\n",
					portid);

		// Start device 
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, portid);

		printf("done: \n");
		if (promiscuous_on) {
			ret = rte_eth_promiscuous_enable(portid);
			if (ret != 0)
				rte_exit(EXIT_FAILURE,
					"rte_eth_promiscuous_enable:err=%s, port=%u\n",
					rte_strerror(-ret), portid);
		}

		
		// print out mac
		struct rte_ether_addr mac_addr;
		rte_eth_macaddr_get(portid, &mac_addr);
		printf("Port %u, MAC address: " RTE_ETHER_ADDR_PRT_FMT "\n\n", portid, RTE_ETHER_ADDR_BYTES(&mac_addr));

		// set net_stack interface properties
		if(!first_port_initialized) {
			interface->port = portid; 
			memcpy(interface->mac, mac_addr.addr_bytes, ETHERNET_MAC_SIZE);
			interface->ipv4_addr = 0;
			first_port_initialized = true;
		}
	}

	check_all_ports_link_status();

	ret = 0;

	/****************** INITIALIZATION DONE. START WORKERS AND OFFLOADER ********************************* */

	// fixed thread count for now on same socket 
	const uint8_t NUM_WORKERS = 3;
	struct execution_context_t** workers = (struct execution_context_t**) NET_STACK_MALLOC("execution_contexts", sizeof(struct execution_context_t*));	;
	struct offloader_t* offloader;

	// launch all workers
	uint8_t worker_idx = 0;
	RTE_LCORE_FOREACH_WORKER(lcore_id) {		
		workers[worker_idx] = create_netstack_execution_context(dpdk_packet_get_packet_buffer, dpdk_packet_free_packet);
		struct execution_context_t* execution_context = workers[worker_idx];
		execution_context->start(execution_context);
				
		if(++worker_idx == NUM_WORKERS) {
			break;
		}
	}

	// create handlers 
	struct handler_config_t *handler_config = (struct handler_config_t*) NET_STACK_MALLOC("Handler Config", sizeof(struct handler_config_t));	
	handler_config->write = handler_write;
	struct handler_t** root_handlers = handler_create_stacks(handler_config);

	// use main lcore (offloader) to distribute packages to exection contexts	
	offloader_loop(interface, root_handlers);

	// wait for cores to stop
	RTE_LCORE_FOREACH_WORKER(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	// close ports
	RTE_ETH_FOREACH_DEV(portid) {
		printf("Closing port %d...", portid);
		ret = rte_eth_dev_stop(portid);
		if (ret != 0)
			printf("rte_eth_dev_stop: err=%d, port=%d\n",
			       ret, portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}

	// clean up the EAL
	rte_eal_cleanup();
	printf("Bye...\n");

	return ret;
}




// int worker_start_lcore_worker(void* setups) {
// 	unsigned lcore_id = rte_lcore_id();

// 	struct lcore_setup_t* lcore_setup = (struct lcore_setup_t*) setups;

// 	for (uint8_t i = 0; i < lcore_setup->num_handlers; i++) {
// 		struct handler_t* handler = lcore_setup->handlers[i];
// 		handler->init(handler, 0);
// 	}
	
// 	worker_main_loop(lcore_setup);
	
// 	for (uint8_t i = 0; i < lcore_setup->num_handlers; i++) {
// 		struct handler_t* handler = lcore_setup->handlers[i];
// 		handler->close(handler);
// 	}

// 	return 0;	
// }