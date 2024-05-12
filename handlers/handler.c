#include <stdint.h>
#include "worker.h"
#include "handlers/handler.h"


int add_value(struct key_val_t* key_vals, uint64_t key, void* value) {
    for (size_t i = 0; key_vals[i].key != 0 ; i++) {
        struct key_val_t* key_val = &key_vals[i];

        if(!key_val->key) {
            struct key_val_t temp = { 
                .key = key,
                .priority = 0,
                .value = value
            };
            *key_val = temp;
            return 0;
        }
    }   
    return -1;     
}

void* get_value(struct key_val_t* key_vals, uint16_t key) {
    for (uint8_t i = 0 ; key_vals[i].key != 0 ; i++) {
        struct key_val_t* key_val = &key_vals[i];

        if(key_val->key == key) {
            key_val->priority++;

            struct key_val_t* previous = &key_vals[i - 1];

            if(i > 0 && key_val->priority > previous->priority) {
                struct key_val_t temp = *previous;
                *previous = *key_val;
                *key_val = temp;
            }

            return key_vals[i].value;
        }
    }
    return 0;    
}


