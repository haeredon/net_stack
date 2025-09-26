#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>

struct interface_operations_t {
    int64_t (*write)(struct out_buffer_t* out_buffer); // write to hardware level
};

struct interface_t {
     uint16_t port;
     uint32_t ipv4_addr; // so far expressed in network order
     uint8_t mac[6];
     struct interface_operations_t operations;
};

struct interface_t* interface_create_interface(struct interface_operations_t interface_operations);

#endif // INTERFACE_H