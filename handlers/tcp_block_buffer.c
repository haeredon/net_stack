#include "tcp_block_buffer.h"
#include "log.h"

#include <string.h>
#include <stdbool.h>

struct tcp_block_t* tcp_block_buffer_add(struct tcp_block_buffer_t* block_buffer, const uint32_t sequence_num, 
                                         const void* data, const uint32_t size) {
    struct tcp_block_t* new_block = block_buffer->free_list;
    
    // if no free blocks are available, allocate some new too a limit
    if(!new_block) {
        // GET SOME MORE BUFFER SPACE, IF IT IS FEASIBLE
        NETSTACK_LOG(NETSTACK_WARNING, "No block on tcp free list.\n");     
        return 0;
    }

    // remove new_block from free list
    block_buffer->free_list = new_block->next;

    // initialize new block
    new_block->size = size;
    new_block->sequence_num = sequence_num;
    new_block->next = 0;

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

struct tcp_block_t* tcp_block_buffer_flush(struct tcp_block_buffer_t* block_buffer) {
    struct tcp_data_block_t* flushed; // must malloc somehow !!!!!
    uint8_t* flush_data = (uint8_t*) (flushed + 1);
    flushed->size = 0;

    struct tcp_block_t* block = block_buffer->blocks;
    uint8_t is_contiguous = true;

    while(block && is_contiguous) {
        flushed->size += block->size;

        // rest of loop simply check if the next block in the buffer is in contiguous space with the 
        // current block, or if there is a gap. If no gap is found then we need to run the loop again 
        // to copy more data
        struct tcp_block_t* next_block = block->next;
        is_contiguous = next_block ? next_block->sequence_num - block->sequence_num == block->size : false;

        if(is_contiguous) {
            block = block->next;
        }        
    }

    block = block_buffer->blocks;
    uint32_t copy_size = flushed->size;    
    while(copy_size) {
        memcpy(flush_data, block->data, block->size);
        flush_data += block->size;
        copy_size -= block->size;
        
        // set next block to be the head of allocated blocks
        block_buffer->blocks = block->next;
        
        // move current block to free list
        block->next = block_buffer->free_list;
        block_buffer->free_list = block;        

        // set next block to be copied from
        block = block_buffer->blocks;
    }
}


// struct tcp_block_t* tcp_block_buffer_add(struct tcp_block_buffer_t* block_buffer, const uint32_t sequence_num, 
//                                          const void* data, const uint32_t size) {
//     if(block_buffer->free_size < size) {
//         NETSTACK_LOG(NETSTACK_WARNING, "New incoming Packet to large for TCP buffer\n");     
//         return 0;
//     }

//     uint32_t offset = block_buffer->size % sequence_num;
    
//     struct tcp_block_t* block = block_buffer->begin;
//     struct tcp_block_t* new_block = (struct tcp_block_t*) &block_buffer->block_array[offset];;
//     new_block->size = size; // this is the size of the data, not the data and block meta data
    
//     // find the blocks which the new block should be in between
//     // if block is null. Then new_block must have next set to whatever comes after beginning. 
//     // If there's a next or more, then previous must have next set to new_block
//     struct tcp_block_t* previous_block = block_buffer->previous;
//     struct tcp_block_t* next_block = block_buffer->next;
//     if(block) {
//         previous_block = block;
//         next_block = block->next;
//         while(next_block && sequence_num > next_block->sequence_num) {
//             previous_block = next_block;
//             next_block = next_block->next;        
//         }
//     } 

//     // make the new block point to the right next block, and make the previous block 
//     // point to the new block
//     new_block->next = next_block;
//     previous_block->next = new_block; 
    
//     // if the new block wraps around the circular buffer. Then we need to do 2 memcpy, 
//     // 1 at the end of the array and 1 at the beginning at the array, to store the entire block
//     if(offset + size > block_buffer->size) {
//         uint32_t first_block_size = block_buffer->size - offset;
//         uint32_t second_block_size = size - second_block_size;
//         memcpy(block_buffer->block_array + offset, data, first_block_size);
//         memcpy(block_buffer->block_array, data, second_block_size);
//     } 
//     // The block does not wrap the circular buffer, so a simple write of the block is enough
//     else {        
//         memcpy(block_buffer->block_array + offset, data, size);
//     }

//     block_buffer->free_size -= size;

//     return new_block;
// } 


// struct tcp_block_t* tcp_block_buffer_flush(struct tcp_block_buffer_t* block_buffer) {
//     struct tcp_data_block_t* flushed; // must malloc somehow !!!!!
//     flushed->size = 0;

//     struct tcp_block_t* block = block_buffer->begin;
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

//     block = block_buffer->begin;
//     uint32_t copy_size = flushed->size;    
//     while(copy_size) {
//         memcpy(flushed + 1, block + 1, block->size);
//         copy_size -= block->size;
//         block = block->next;
//     }

//     block_buffer->free_size += flushed->size;
//     block_buffer->begin = 
//     block_buffer->previous = 
//     block_buffer->next = 
    
//     // clear up next pointers
// }


struct tcp_block_buffer_t* create_tcp_block_buffer() {

}