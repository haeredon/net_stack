#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

#include "interface.h"
#include "handlers/types.h"


#define DEFAULT_PACKAGE_BUFFER_SIZE 0xFFFF

struct request_t {
    void* buffer;
    uint64_t size;
};


struct response_buffer_t {
    void* buffer;
    uint64_t size;
    uint64_t offset;
    uint8_t stack_idx;
};

struct response_t;
struct handler_t;

struct in_buffer_t {
    const void* packet_pointers[10];
};


struct in_packet_stack_t {
    struct handler_t* handlers[10];
    void* return_args[10];

    struct in_buffer_t in_buffer;
    uint8_t stack_idx;
};

struct out_packet_stack_t {
    struct handler_t* handlers[10];
    void* args[10];

    struct out_buffer_t out_buffer;
    uint8_t stack_idx;
};

struct transmission_config_t {

};


struct response_t {
    void* buffer;
    uint64_t size;    
    struct interface_t* interface;
};



struct operations_t {
    bool (*write)(struct out_packet_stack_t* packet_stack, struct interface_t* interface, const struct handler_t* handler);            
    // reading incoming packets    
    uint16_t (*read)(struct in_packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler);
};

struct handler_config_t {    
    // write callback for handler, that's writing to some interface
    uint16_t (*write)(struct out_buffer_t* buffer, struct interface_t* interface, struct transmission_config_t* transmission_config); 
};

struct handlers_t {
    struct handler_t* handlers[10];
    uint8_t size;
};

struct handler_t {
    void (*init)(struct handler_t*, void* priv_config); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    struct handler_config_t *handler_config;

    void* priv;
};


struct out_packet_stack_t* handler_create_out_package_stack(struct in_packet_stack_t* packet_stack, uint8_t package_depth);
struct handler_t** handler_create_stacks(struct handler_config_t *config);
uint16_t handler_write(struct out_buffer_t* buffer, struct interface_t* interface, struct transmission_config_t* transmission_config);


#endif // HANDLER_HANDLER_H