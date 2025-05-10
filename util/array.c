#include "array.h"

#include <stdbool.h>

bool array_is_zero(void* array, uint64_t size) {
    const uint8_t* to_cmp = (const uint8_t*) array;

    while(size--) {
        if(to_cmp[size]) {
            return false;
        }
    }
    return true;
}

