#include "tcp_out_buffer.h"

#include "log.h"

#include <string.h>
#include <stdbool.h>
#include <time.h>

struct tcp_block_t* tcp_block_buffer_add(struct tcp_out_buffer_t* out_buffer, const uint32_t sequence_num, 
                                         const void* data, const uint32_t size) {
    struct tcp_out_block_t* new_block = out_buffer->free_list;
    
    new_block->sequence_num = sequence_num;
    new_block->timestamp = time(0); 
    new_block->data = data;
    new_block->num_retransmissions = 0;
    new_block->next = 0;
    
    // if no free blocks are available, block it
    if(!new_block) {
        NETSTACK_LOG(NETSTACK_WARNING, "No block on tcp free list.\n");     
        return 0;
    }

    out_buffer->free_list = new_block->next;

    struct tcp_out_block_t* block = out_buffer->blocks;
    
    if(block) {
        while(block->next) {
            block = block->next;
        }

        block->next = new_block;
    } else {
        out_buffer->blocks = new_block;
    }

    return new_block;
} 
