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

#include "pcapng.h"
#include "worker.h"


void pcapng_init_handler(struct handler_t* handler) {
    struct kage_t* kage = (struct kage_t*) rte_zmalloc("pcap handler private data", sizeof(struct kage_t), 0); 

    // Get a file descriptor to store the pcapng data in
    char* file = "kage.pcap"; // TODO: get a way of using a more standardized way of defining file names
    int fd = open(file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);


    // get pcapng file descriptor which is an abstraction on top of the normal file descriptor
    struct rte_pcapng* pcap_fd = rte_pcapng_fdopen(fd, NULL, NULL, NULL, NULL);

    if(pcap_fd == NULL) {
        RTE_LOG(ERR, USER1, "Could not get pcapng file descriptor, rte_errno: %u\n", rte_errno);        
    }

    // information about the interface and used port must be added to the pcapng file descriptor. If not
    // done, we can't write to the pcang file descriptor
    if(rte_pcapng_add_interface(pcap_fd, 0, NULL, NULL, NULL) < 0) {
        RTE_LOG(ERR, USER1, "Could not add interface information to pcapng file descriptor\n");        
    }

    // create mempool for to be used when flushing packages to pcap ng
    struct rte_mempool* pool = rte_pktmbuf_pool_create("pcap_pool", 8192U, 256, 0, 
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if(pool == NULL) {
        RTE_LOG(ERR, USER1, "Could not create mempool for pcapng, rte_errno: %u\n", rte_errno);        
    }

    kage->pcap_fd = pcap_fd;
    kage->mem_pool = pool;

    handler->priv = (void*) kage;
}

void pcapng_close_handler(struct handler_t* handler) {
    struct kage_t* kage = (struct kage_t*) handler->priv;
    rte_pcapng_close(kage->pcap_fd);
    rte_free(kage);
}


uint16_t pcapng_read(struct rte_mbuf* buffer, struct interface_t* interface, void* priv) {
    struct kage_t* kage = (struct kage_t*) priv;

    struct rte_mbuf* pcap_buffers[1];
    
    pcap_buffers[0] = rte_pcapng_copy(interface->port, interface->queue, buffer, kage->mem_pool, 8192U, RTE_PCAPNG_DIRECTION_IN, "");

    if(pcap_buffers[0] == NULL) {
        RTE_LOG(ERR, USER1, "Could not copy incoming package to pcapng, rte_errno: %u\n", rte_errno);        
    }

    int num_written = rte_pcapng_write_packets(kage->pcap_fd, pcap_buffers, 1);

    if(num_written < 0) {
        RTE_LOG(ERR, USER1, "Failed to write pcapng packages to file, rte_errno: %u\n", rte_errno);        
        return -1;
    }

    return 0;
}

struct handler_t* pcapng_create_handler() {
    struct handler_t* handler = (struct handler_t*) rte_zmalloc("pcapng handler", sizeof(struct handler_t), 0);	

    handler->init = pcapng_init_handler;
    handler->close = pcapng_close_handler;

    handler->operations.read = pcapng_read;
}