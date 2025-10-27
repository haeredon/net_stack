#include "handlers/tcp/tcp_block_buffer.h"
#include "util/log.h"
#include "util/memory.h"

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

        while(block && new_block->sequence_num >= block->sequence_num) {
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

struct tcp_block_t* tcp_block_buffer_get_head(struct tcp_block_buffer_t* block_buffer) {
    return block_buffer->blocks;
}

struct tcp_block_t* tcp_block_buffer_destroy(struct tcp_block_buffer_t* block_buffer) {

}

struct tcp_block_buffer_t* create_tcp_block_buffer(uint16_t max_size) {
    struct tcp_block_buffer_t* block_buffer = (struct tcp_block_buffer_t*) NET_STACK_MALLOC("tcp block buffer", sizeof(struct tcp_block_buffer_t));

    block_buffer->free_list = NET_STACK_MALLOC("tcp block buffer: free_list", sizeof(struct tcp_block_t) * TCP_BLOCK_BUFFER_DEFAULT_SIZE); 
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