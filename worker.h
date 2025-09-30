#ifndef WORKER_H
#define WORKER_H

#include "handlers/handler.h"
#include "util/queue.h"

#include<stdint.h>
#include<rte_mbuf.h>
#include<pthread.h>

#define MAX_PKT_BURST 32
#define QUEUE_SIZE 64

#ifdef RTE_THREAD_MANAGEMENT
    #define EXECUTION_CONTEXT_THREAD_HANDLE uint64_t
    #define EXECUTION_CONTEXT_THREAD_CREATE(HANDLE, CALL_FUN, ARG) rte_eal_remote_launch(CALL_FUN, ARG, HANDLE);
    #define EXECUTION_CONTEXT_THREAD_STOP(HANDLE) rte_eal_wait_lcore(HANDLE)
#else
    #define EXECUTION_CONTEXT_THREAD_HANDLE pthread_t
    #define EXECUTION_CONTEXT_THREAD_CREATE(HANDLE, CALL_FUN, ARG) pthread_create(&HANDLE, NULL, CALL_FUN, ARG);
    #define EXECUTION_CONTEXT_THREAD_STOP(HANDLE) pthread_join(HANDLE, NULL)
#endif




struct lcore_setup_t {
    struct interface_t interface;
    struct handler_t** handlers;
    uint8_t num_handlers;
};

int worker_start_lcore_worker(void* setups);

extern volatile bool force_quit; 


/************************************************************************************ */
struct task_t {
    void* argument;
    void (*run)(void* argument);            
};

enum Execution_state {
    NET_STACK_STARTING,
    NET_STACK_STOPPED,
    NET_STACK_STOPPING,
    NET_STACK_RUNNING
}; 

struct execution_context_t {
    QUEUE_TYPE* work_queue;        

    EXECUTION_CONTEXT_THREAD_HANDLE thread_handle;

    enum Execution_state state;
    pthread_mutex_t state_lock;

    void (*stop)(struct execution_context_t* execution_context);
    int (*start)(struct execution_context_t* execution_context);

    void* (*get_packet_buffer)(void* packet);
	void (*free_packet)(void* packet);	
};

struct execution_context_t* create_netstack_execution_context(void* (*get_packet_buffer)(void* packet),
												   			  void (*free_packet)(void* packet));


#endif // WORKER_H