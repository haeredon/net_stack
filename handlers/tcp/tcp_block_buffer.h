#ifndef HANDLERS_TCP_BLOCK_BUFFER_H
#define HANDLERS_TCP_BLOCK_BUFFER_H

#include <stdint.h>
#include <stddef.h>

#define TCP_BLOCK_BUFFER_DEFAULT_SIZE 10

struct tcp_data_block_t {
    uint32_t size;  
};

struct tcp_block_t {
    uint32_t size;  
    uint32_t sequence_num;  
    void* data;
    struct tcp_block_t* next;
};

struct tcp_block_buffer_t {
    struct tcp_block_t* free_list;
    struct tcp_block_t* blocks;

    void* (*mem_allocate)(const char *type, size_t size);
    void (*mem_free)(void*);

    uint32_t max_size;
};

// struct tcp_data_block_t* tcp_block_buffer_flush(struct tcp_block_buffer_t* block_buffer);

struct tcp_block_t* tcp_block_buffer_remove_front(struct tcp_block_buffer_t* block_buffer, const uint16_t num_to_remove);

uint16_t tcp_block_buffer_num_ready(struct tcp_block_buffer_t* block_buffer);

struct tcp_block_t* tcp_block_buffer_add(struct tcp_block_buffer_t* block_buffer, const uint32_t sequence_num, 
                                         const void* data);

struct tcp_block_buffer_t* create_tcp_block_buffer(uint16_t max_size,  void* (*mem_allocate)(const char *type, size_t size),
                                                   void (*mem_free)(void*));




#endif // HANDLERS_TCP_BLOCK_BUFFER_H