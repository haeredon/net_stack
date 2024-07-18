#ifndef HANDLER_PROTOCOL_MAP_H
#define HANDLER_PROTOCOL_MAP_H

#include "handler.h"

#define NUM_HANDLERS 32


struct key_val_t {
    uint64_t key;
    uint64_t priority;
    void* value;
};

struct priority_map_t {
    uint16_t max_size;
    struct key_val_t* map;
};


void* get_value(struct priority_map_t* map, uint16_t key);
int add_value(struct priority_map_t* map, uint64_t key, void* value);


#define ADD_TO_PRIORITY(KEYS_TO_VALS, KEY, VALUE) ( \
    add_value(KEYS_TO_VALS, KEY, (void*) VALUE) \
)

#define GET_FROM_PRIORITY(KEYS_TO_VALS, KEY, TYPE) ( \
    (TYPE*) get_value(KEYS_TO_VALS, KEY) \
)



#endif // HANDLER_PROTOCOL_MAP_H



/******************* Initialization ************************/
// 1. register response handlers for all handlers
    // maybe use priority queue for this?

/***************** Handling (writing) *******************/
// 1. add own reponse handler to packet_stack object
// 2. increment write_chain_lenght
// 3. get a buffer to write to         
// 4. pass the response buffer to the response handler stack


/****************** Handling (not writing) *****************************/
// 1. add own reponse handler to packet_stack object
// 2. increment write_chain_lenght
// 3. pass to next in chain

// remember that the structure packet_stack_t at the beginning of read function always must have 
// write_chain_length set to the index the packet_pointer which the handler should work on.
// before leaving the read function write_chain_length must be incremented and the index in 
// must be set to point to the next data in the incoming buffer 