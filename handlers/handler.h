#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H


#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


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

/********************************** */

struct new_in_buffer_t {
    const void* packet_pointers[10];
    uint64_t size;
    uint64_t offset;  
};

struct new_out_buffer_t {
    void* buffer;
    uint64_t size;
    uint64_t offset;
};

struct new_in_packet_stack_t {
    const struct handler_t* handlers[10];

    const struct new_in_buffer_t in_buffer;
    uint8_t stack_idx;
};

struct new_out_packet_stack_t {
    const struct handler_t* handlers[10];

    struct new_out_buffer_t out_buffer;
    uint8_t stack_idx;
};


/********************************** */

struct response_t;
struct interface_operations_t {
    int64_t (*write)(struct response_t response); // write to hardware level
};

struct interface_t {
     uint16_t port;
     uint32_t queue;
     uint32_t ipv4_addr; // so far expressed in network order
     uint8_t mac[6];
     struct interface_operations_t operations;
};

struct handler_t;

struct packet_stack_t {
    // should be called iterative as part of a response chain that is iterated over by a handler.c function
    uint16_t (*pre_build_response[10])(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, const struct interface_t* interface);
    void (*post_build_response[10])(struct packet_stack_t* packet_stack, struct response_buffer_t* response_buffer, 
                                  const struct interface_t* interface, uint16_t offset);
    const void* packet_pointers[10];
    const struct handler_t* handlers[10];
    uint8_t write_chain_length;
};

struct transmission_config_t {

};

struct package_buffer_t {
    void* buffer;
    uint64_t size;
    uint64_t data_offset;
};

struct response_t {
    void* buffer;
    uint64_t size;    
    struct interface_t* interface;
};



struct operations_t {
    bool (*write)(struct packet_stack_t* packet_stack, struct package_buffer_t* buffer, uint8_t stack_idx, struct interface_t* interface, const struct handler_t* handler);            
    // reading incoming packets    
    uint16_t (*read)(struct packet_stack_t* packet_stack, struct interface_t* interface, struct handler_t* handler);

    // get necessary information from a given package, to create a response to it. Can be used in conjunction with the write function
    void* (*get_response_args_from_incoming_package)();
};

struct handler_config_t {
    void* (*mem_allocate)(const char *type, size_t size);
    void (*mem_free)(void*);
    
    // write callback for handler, that's writing to some interface
    uint16_t (*write)(struct package_buffer_t* buffer, struct interface_t* interface, struct transmission_config_t* transmission_config); 
};

struct handler_t {
    void (*init)(struct handler_t*, void* priv_config); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    struct handler_config_t *handler_config;

    void* priv;
};

uint16_t handler_response(struct packet_stack_t* packet_stack, struct interface_t* interface, struct transmission_config_t* transmission_config);
struct handler_t** handler_create_stacks(struct handler_config_t *config);


#endif // HANDLER_HANDLER_H