#include "tcp_block_buffer.h"
#include "log.h"

#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

struct tcp_block_t* tcp_block_buffer_add(struct tcp_block_buffer_t* block_buffer, void* data, 
    const uint32_t sequence_num, uint16_t tcp_payload_size) {
    
    struct tcp_block_t* new_block = block_buffer->free_list;
    
    // if no free blocks are available, block it
    if(!new_block) {
        NETSTACK_LOG(NETSTACK_WARNING, "No block on tcp free list.\n");     
        return 0;
    }

    // remove new_block from free list
    block_buffer->free_list = new_block->next;

    // initialize new block
    new_block->data = data;
    new_block->sequence_num = ntohl(sequence_num);
    new_block->next = 0;
    new_block->payload_size = tcp_payload_size;

    // Find the blocks coming right after and right after the new block 
    // in relation to sequence numbers. Place the new block between these 
    // 2 blocks
    struct tcp_block_t* block = block_buffer->blocks;
    if(block) {
        struct tcp_block_t* previous_block = 0;
        struct tcp_block_t* next_block = block->next;

        while(block && new_block->sequence_num > block->sequence_num) {
            previous_block = block;
            block = block->next;
        }

        new_block->next = block;
        previous_block->next = new_block;
    } else {
        // if no blocks exist, then new_block is the only block and the head of the list
        block_buffer->blocks = new_block;
    }

    return new_block;
} 

bool tcp_block_buffer_remove_front(struct tcp_block_buffer_t* block_buffer, uint16_t num_to_remove) {
    struct tcp_block_t* block = block_buffer->blocks;
    
    while(num_to_remove-- && block) {
        block_buffer->blocks = block->next;
        
        block->next = block_buffer->free_list;
        block_buffer->free_list = block;

        block = block_buffer->blocks;
    }
}

uint16_t tcp_block_buffer_num_ready(struct tcp_block_buffer_t* block_buffer, uint32_t start_sequence_num) {
    uint16_t num_ready = 0;
    struct tcp_block_t* block = block_buffer->blocks;

    while(block && 
        ((block->sequence_num == start_sequence_num) ||
        (block->sequence_num == start_sequence_num + 1) /* allow for acks */)) {
        
        num_ready++;
        start_sequence_num = block->sequence_num + block->payload_size;
        block = block->next;        
    }

    return num_ready;
}



// struct tcp_data_block_t* tcp_block_buffer_flush(struct tcp_block_buffer_t* block_buffer) {
//     struct tcp_data_block_t* flushed = (struct tcp_data_block_t*) block_buffer->mem_allocate("tcp block buffer: flush data block", 
//                                             sizeof(struct tcp_data_block_t));

//     uint8_t* flush_data = (uint8_t*) (flushed + 1);
//     flushed->size = 0;

//     struct tcp_block_t* block = block_buffer->blocks;
//     uint8_t is_contiguous = true;

//     while(block && is_contiguous) {
//         flushed->size += block->size;

//         // rest of loop simply check if the next block in the buffer is in contiguous space with the 
//         // current block, or if there is a gap. If no gap is found then we need to run the loop again 
//         // to copy more data
//         struct tcp_block_t* next_block = block->next;
//         is_contiguous = next_block ? next_block->sequence_num - block->sequence_num == block->size : false;

//         if(is_contiguous) {
//             block = block->next;
//         }        
//     }

//     block = block_buffer->blocks;
//     uint32_t copy_size = flushed->size;    
//     while(copy_size) {
//         memcpy(flush_data, block->data, block->size);
//         flush_data += block->size;
//         copy_size -= block->size;
        
//         block_buffer->mem_free(block->data);

//         // set next block to be the head of allocated blocks
//         block_buffer->blocks = block->next;
        
//         // move current block to free list
//         block->next = block_buffer->free_list;
//         block_buffer->free_list = block;        

//         // set next block to be copied from
//         block = block_buffer->blocks;
//     }
// }


struct tcp_block_t* tcp_block_buffer_get_head(struct tcp_block_buffer_t* block_buffer) {
    return block_buffer->blocks;
}

struct tcp_block_t* tcp_block_buffer_destroy(struct tcp_block_buffer_t* block_buffer, void (*mem_free)(void*)) {

}

struct tcp_block_buffer_t* create_tcp_block_buffer(uint16_t max_size,  void* (*mem_allocate)(const char *type, size_t size),
                                                   void (*mem_free)(void*)) {
    struct tcp_block_buffer_t* block_buffer = (struct tcp_block_buffer_t*) mem_allocate("tcp block buffer", sizeof(struct tcp_block_buffer_t));

    block_buffer->mem_allocate = mem_allocate;
    block_buffer->mem_free = mem_free;

    block_buffer->free_list = block_buffer->mem_allocate("tcp block buffer: free_list", sizeof(struct tcp_block_t) * TCP_BLOCK_BUFFER_DEFAULT_SIZE); 
    block_buffer->blocks = 0;

    // initialize free list
    uint16_t block_buffer_iter = TCP_BLOCK_BUFFER_DEFAULT_SIZE;
    struct tcp_block_t* block = &block_buffer->free_list[block_buffer_iter];
    block->next = 0;
    while(block_buffer_iter--) {
        struct tcp_block_t* prev_block = &block_buffer->free_list[block_buffer_iter];
        prev_block->next = block;
        block = prev_block;
    }

    block_buffer->max_size = max_size;

    return block_buffer;
}