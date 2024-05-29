#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H


#include <stdint.h>

#include <rte_mbuf.h>


struct interface_t {
     uint16_t port;
     uint32_t queue;
};

struct operations_t {
    uint16_t (*read)(struct rte_mbuf* buffer, uint16_t offset, struct interface_t* interface, void* priv);    
};

struct handler_t {
    void (*init)(struct handler_t*); 
    void (*close)(struct handler_t*);
    struct operations_t operations;

    void* priv;
};

uint8_t handler_init(struct handler_t** handlers);


#endif // HANDLER_HANDLER_H