#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H


#include <stdint.h>
#include <stddef.h>


struct response_buffer_t {
    void* buffer;
    uint64_t size;
    uint64_t offset;
    uint8_t stack_idx;
};

struct packet_stack_t {
    // should be called iterative as part of a response chain that is iterated over by a handler.c function
    uint16_t (*response[10])();
    void* packet_pointers[10];
    uint8_t write_chain_length;
};

struct interface_operations_t {
    int64_t (*write)(void* buffer, uint64_t size);        
};

struct interface_t {
     uint16_t port;
     uint32_t queue;
     struct interface_operations_t operations;
};

struct operations_t {
    uint16_t (*read)(struct packet_stack_t* packet_stack, struct interface_t* interface, void* priv);        
    uint16_t (*response)(struct packet_stack_t* packet_stack, struct response_buffer_t response_buffer, struct interface_t* interface);        
};

struct handler_config_t {
    void* (*mem_allocate)(const char *type, size_t size);
    void (*mem_free)(void*);
};

struct handler_t {
    void (*init)(struct handler_t*); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    struct handler_config_t *handler_config;

    void* priv;
};

uint16_t handler_response(struct packet_stack_t* packet_stack, struct interface_t* interface);
struct handler_t** handler_create_stacks(struct handler_config_t *config);


#endif // HANDLER_HANDLER_H