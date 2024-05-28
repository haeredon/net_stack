#include <stdint.h>
#include "worker.h"
#include "handlers/protocol_map.h"


int add_value(struct priority_map_t* map, uint64_t key, void* value) {
    struct key_val_t* key_vals = map->map;

    for (size_t i = 0; i < map->max_size ; i++) {
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

void* get_value(struct priority_map_t* map, uint16_t key) {
    struct key_val_t* key_vals = map->map;

    for (uint8_t i = 0 ; i < map->max_size && key_vals[i].key != 0 ; i++) {
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


