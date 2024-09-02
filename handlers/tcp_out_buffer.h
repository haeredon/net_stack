#ifndef HANDLERS_TCP_OUT_BUFFER_H
#define HANDLERS_TCP_OUT_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define TCP_OUT_BUFFER_DEFAULT_SIZE 10

struct tcp_out_block_t {    
    uint32_t sequence_num;
    time_t timestamp;  
    void* data;
    uint8_t num_retransmissions;
    struct tcp_out_block_t* next;
};

struct tcp_out_buffer_t {
    struct tcp_out_block_t* free_list;
    struct tcp_out_block_t* blocks;

    void* (*mem_allocate)(const char *type, size_t size);
    void (*mem_free)(void*);

    uint32_t max_size;
};

struct tcp_block_t* tcp_out_buffer_add(struct tcp_block_buffer_t* block_buffer, const uint32_t sequence_num, 
                                         const void* data, const uint32_t size);

struct tcp_block_buffer_t* create_tcp_out_buffer(uint16_t max_size,  void* (*mem_allocate)(const char *type, size_t size),
                                                   void (*mem_free)(void*));


#endif // HANDLERS_TCP_OUT_BUFFER_H