#ifndef HANDLERS_TCP_BLOCK_BUFFER_H
#define HANDLERS_TCP_BLOCK_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TCP_BLOCK_BUFFER_DEFAULT_SIZE 10

struct tcp_block_t {
    uint32_t payload_size;  
    uint32_t sequence_num;  
    void* data;
    struct tcp_block_t* next;
};

struct tcp_block_buffer_t {
    struct tcp_block_t* free_list;
    struct tcp_block_t* last_free;
    struct tcp_block_t* blocks;

    uint32_t max_size;
};


struct tcp_block_t* tcp_block_buffer_destroy(struct tcp_block_buffer_t* block_buffer);

bool tcp_block_buffer_remove_front(struct tcp_block_buffer_t* block_buffer, const uint16_t num_to_remove);

struct tcp_block_t* tcp_block_buffer_get_head(struct tcp_block_buffer_t* block_buffer);

uint16_t tcp_block_buffer_num_ready(struct tcp_block_buffer_t* block_buffer, uint32_t start_sequence_num);

struct tcp_block_t* tcp_block_buffer_add(struct tcp_block_buffer_t* block_buffer, void* data, const uint32_t sequence_num, uint16_t tcp_payload_size);

struct tcp_block_buffer_t* create_tcp_block_buffer(uint16_t max_size);




#endif // HANDLERS_TCP_BLOCK_BUFFER_H