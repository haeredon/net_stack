#ifndef HANDLER_PROTOCOL_MAP_H
#define HANDLER_PROTOCOL_MAP_H

#include "worker.h"

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