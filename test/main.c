#include "pcapng.h"
#include "handlers/handler.h"
#include "handlers/arp.h"
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_packet.h>


struct test_t {
    uint8_t (*test)(struct test_t* test);
    int (*init)(struct test_t* test, struct handler_t* handler);
    void (*end)(struct test_t* test);

    struct pcapng_reader_t* reader;
    struct handler_t* handler;
};


uint8_t test_test(struct test_t* test) {
    static const int MAX_BUFFER_SIZE = 0xFFFF;
    uint8_t* req_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    uint8_t* res_buffer = (uint8_t*) malloc(MAX_BUFFER_SIZE);
    struct pcapng_reader_t* reader = test->reader;

    do {
        if(reader->read_block(reader, req_buffer, MAX_BUFFER_SIZE) == -1)  {
            printf("FAIL.\n");	
        }
        
        if(packet_is_type(req_buffer, PCAPNG_ENHANCED_BLOCK)) {
            printf("Enhanced Block!\n");    

            struct packet_enchanced_block_t* block = (struct packet_enchanced_block_t*) req_buffer;

            test->handler->operations.read(&block->packet_data, 0, 0, test->handler->priv);            

            printf("Do test!\n");
        }
       

    } while(reader->has_more_headers(reader));
}

int test_init(struct test_t* test, struct handler_t* handler) {
    test->reader = create_pcapng_reader();        

    if(test->reader->init(test->reader, "test.pcapng")) {
        test->reader->close(test->reader);
        return -1;
    };    
    
    test->handler = handler;
    test->handler->init(test->handler);

    return 0;
}

void test_end(struct test_t* test) {
    test->reader->close(test->reader);
    test->handler->close(test->handler);
}

struct test_t* create_test_suite() {
    struct test_t* test = (struct test_t*) malloc(sizeof(struct test_t));

    test->init = test_init;
    test->end = test_end;
    test->test = test_test;    
}

/**********************************************/

void* test_malloc(const char *type, size_t size, unsigned align) {
    return malloc(size);
}

int main(int argc, char **argv) {    
    struct handler_t** handlers = handler_init(test_malloc);        
    
    // start testing
    struct test_t* test_suite = create_test_suite();

    if(test_suite->init(test_suite, handlers[1])) { // we know ethernet is index 1. That needs to be made more stable in the future
        return -1;
    }

    test_suite->test(test_suite);
    test_suite->end(test_suite);
}





// int init_eal() {
//     /* Init EAL. */
// 	ret = rte_eal_init(argc, argv);
// 	if (ret < 0)
// 		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
// 	argc -= ret;
// 	argv += ret;

// 	signal(SIGINT, signal_handler);
// 	signal(SIGTERM, signal_handler);

// 	num_ports = rte_eth_dev_count_avail();
// 	if (num_ports == 0)
// 		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");
// }


	// int ret;
	// uint16_t nb_ports;
	// uint16_t nb_ports_available = 0;
	// uint16_t portid, last_port;
	// unsigned lcore_id, rx_lcore_id;
	// unsigned nb_ports_in_mask = 0;
	// unsigned int nb_lcores = 0;
	// unsigned int nb_mbufs;



    // uint64_t num_mbufs = 8192U;

	// /* Create the mbuf pool. 8< */
	// l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", nb_mbufs,
	// 	256 /* core local cache */, 0, RTE_MBUF_DEFAULT_BUF_SIZE,
	// 	rte_socket_id());
	// if (l2fwd_pktmbuf_pool == NULL)
	// 	rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
	// /* >8 End of create the mbuf pool. */

	// /* Initialise each port */
	// RTE_ETH_FOREACH_DEV(portid) {
	// 	struct rte_eth_rxconf rxq_conf;
	// 	struct rte_eth_txconf txq_conf;
	// 	struct rte_eth_conf local_port_conf = port_conf;
	// 	struct rte_eth_dev_info dev_info;

	// 	/* skip ports that are not enabled */
	// 	if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
	// 		printf("Skipping disabled port %u\n", portid);
	// 		continue;
	// 	}
	// 	nb_ports_available++;

	// 	/* init port */
	// 	printf("Initializing port %u... ", portid);
	// 	fflush(stdout);

	// 	ret = rte_eth_dev_info_get(portid, &dev_info);
	// 	if (ret != 0)
	// 		rte_exit(EXIT_FAILURE,
	// 			"Error during getting device (port %u) info: %s\n",
	// 			portid, strerror(-ret));

	// 	if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
	// 		local_port_conf.txmode.offloads |=
	// 			RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
	// 	/* Configure the number of queues for a port. */
	// 	ret = rte_eth_dev_configure(portid, 1, 1, &local_port_conf);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
	// 			  ret, portid);
	// 	/* >8 End of configuration of the number of queues for a port. */

	// 	ret = rte_eth_dev_adjust_nb_rx_tx_desc(portid, &nb_rxd,
	// 					       &nb_txd);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE,
	// 			 "Cannot adjust number of descriptors: err=%d, port=%u\n",
	// 			 ret, portid);

	// 	ret = rte_eth_macaddr_get(portid,
	// 				  &l2fwd_ports_eth_addr[portid]);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE,
	// 			 "Cannot get MAC address: err=%d, port=%u\n",
	// 			 ret, portid);

	// 	/* init one RX queue */
	// 	fflush(stdout);
	// 	rxq_conf = dev_info.default_rxconf;
	// 	rxq_conf.offloads = local_port_conf.rxmode.offloads;
	// 	/* RX queue setup. 8< */
	// 	ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
	// 				     rte_eth_dev_socket_id(portid),
	// 				     &rxq_conf,
	// 				     l2fwd_pktmbuf_pool);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
	// 			  ret, portid);
	// 	/* >8 End of RX queue setup. */

	// 	/* Init one TX queue on each port. 8< */
	// 	fflush(stdout);
	// 	txq_conf = dev_info.default_txconf;
	// 	txq_conf.offloads = local_port_conf.txmode.offloads;
	// 	ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
	// 			rte_eth_dev_socket_id(portid),
	// 			&txq_conf);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
	// 			ret, portid);
	// 	/* >8 End of init one TX queue on each port. */

	// 	/* Initialize TX buffers */
	// 	tx_buffer[portid] = rte_zmalloc_socket("tx_buffer",
	// 			RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
	// 			rte_eth_dev_socket_id(portid));
	// 	if (tx_buffer[portid] == NULL)
	// 		rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
	// 				portid);

	// 	rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

	// 	ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
	// 			rte_eth_tx_buffer_count_callback,
	// 			&port_statistics[portid].dropped);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE,
	// 		"Cannot set error callback for tx buffer on port %u\n",
	// 			 portid);

	// 	ret = rte_eth_dev_set_ptypes(portid, RTE_PTYPE_UNKNOWN, NULL,
	// 				     0);
	// 	if (ret < 0)
	// 		printf("Port %u, Failed to disable Ptype parsing\n",
	// 				portid);
	// 	/* Start device */
	// 	ret = rte_eth_dev_start(portid);
	// 	if (ret < 0)
	// 		rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
	// 			  ret, portid);

	// 	printf("done: \n");
	// 	if (promiscuous_on) {
	// 		ret = rte_eth_promiscuous_enable(portid);
	// 		if (ret != 0)
	// 			rte_exit(EXIT_FAILURE,
	// 				"rte_eth_promiscuous_enable:err=%s, port=%u\n",
	// 				rte_strerror(-ret), portid);
	// 	}

	// 	printf("Port %u, MAC address: " RTE_ETHER_ADDR_PRT_FMT "\n\n",
	// 		portid,
	// 		RTE_ETHER_ADDR_BYTES(&l2fwd_ports_eth_addr[portid]));

	// 	/* initialize port stats */
	// 	memset(&port_statistics, 0, sizeof(port_statistics));
	// }

	// if (!nb_ports_available) {
	// 	rte_exit(EXIT_FAILURE,
	// 		"All available ports are disabled. Please set portmask.\n");
	// }

	// check_all_ports_link_status(l2fwd_enabled_port_mask);



	// l2fwd_simple_forward(l2fwd_pktmbuf_pool);
	// // struct pcapng_reader_t pcapng_reader = {
	// // 	.init = pcapng_init,
	// // 	.read = pcapng_read2,
	// // 	.has_more_headers = pcapng_has_more_headers
	// // };

	// // send_test_data(&pcapng_reader);


	// ret = 0;
	// /* launch per-lcore init on every lcore */
	// rte_eal_mp_remote_launch(worker_start_lcore_worker, setup, CALL_MAIN);
	// RTE_LCORE_FOREACH_WORKER(lcore_id) {
	// 	if (rte_eal_wait_lcore(lcore_id) < 0) {
	// 		ret = -1;
	// 		break;
	// 	}
	// }

	// RTE_ETH_FOREACH_DEV(portid) {
	// 	if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
	// 		continue;
	// 	printf("Closing port %d...", portid);
	// 	ret = rte_eth_dev_stop(portid);
	// 	if (ret != 0)
	// 		printf("rte_eth_dev_stop: err=%d, port=%d\n",
	// 		       ret, portid);
	// 	rte_eth_dev_close(portid);
	// 	printf(" Done\n");
	// }

	// /* clean up the EAL */
	// rte_eal_cleanup();
	// printf("Bye...\n");

	// return ret;








