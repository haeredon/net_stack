#ifndef HANDLER_HANDLER_H
#define HANDLER_HANDLER_H

#include "worker.h"

#define NUM_HANDLERS 32

struct key_val_t {
    uint64_t key;
    uint64_t priority;
    void* value;
};

void* get_value(struct key_val_t* key_vals, uint16_t key);
int add_value(struct key_val_t* key_vals, uint64_t key, void* value);


#define ADD_TO_PRIORITY(KEYS_TO_VALS, KEY, VALUE) ( \
    add_value(KEY, (void*) VALUE) \
)

#define GET_FROM_PRIORITY(KEYS_TO_VALS, KEY, TYPE) ( \
    (TYPE*) get_value(KEYS_TO_VALS, KEY) \
)



#endif // HANDLER_HANDLER_H