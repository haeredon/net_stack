#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H


#include <stdint.h>

#include <rte_mbuf.h>



struct packet_stack_t {
    // should be called iterative as part of a response chain that is iterated over by a handler.c function
    uint16_t (*response)()[10];
    void* packet_pointers[10];
    uint8_t write_chain_length;
};

struct interface_t {
     uint16_t port;
     uint32_t queue;
};

struct operations_t {
    uint16_t (*read)(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv);        
};

struct handler_t {
    void (*init)(struct handler_t*); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    void* priv;
};

struct handler_t** handler_create_stacks(void* (*mem_allocate)(const char *type, size_t size, unsigned align));


#endif // HANDLER_HANDLER_H