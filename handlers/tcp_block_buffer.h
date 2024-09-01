#ifndef HANDLERS_TCP_BLOCK_BUFFER_H
#define HANDLERS_TCP_BLOCK_BUFFER_H

#include <stdint.h>

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

    uint32_t max_size;
};

struct tcp_block_t* tcp_block_buffer_flush();

struct tcp_block_t* tcp_block_buffer_add();

struct tcp_block_buffer_t* create_tcp_block_buffer();




#endif // HANDLERS_TCP_BLOCK_BUFFER_H



/**
 * circular buffer
 * 
 * struct block {
 *  int size
 *  struct block next
 * }
 * 
 * 
 * Beginning
 *   ||
 *   \/
 * ----------------------------------------
 * | 1 | 300 | 500 |   |   | 1000 |   |   |
 * ----------------------------------------
 * 
 * if continous buffer then 
 *      flush the continous blocks
 *      set beginning of circular buffer to the, posibly empty, block just after the continous blocks
 * 
 * next time a block arrives
 * 
 *          Beginning
 *              ||
 *              \/
 * ----------------------------------------
 * |   |   |   |   |   | 1000 |   |   |
 * ----------------------------------------
 *
 * insert new block
 *            Beginning
 *                ||
 *                \/
 * ----------------------------------------
 * |   |   |   | 600  |   | 1000 |   |   |
 * ----------------------------------------
 * 
  * insert new block
 *             Beginning
 *                ||
 *                \/
 * ----------------------------------------
 * |   |   |   | 600  | 750 | 1000 |   |   |
 * ----------------------------------------
 * 
 * if continous buffer then 
 *      flush the continous blocks
 *      set beginning of circular buffer to the, posibly empty, block just after the continous blocks
 *
 *                     Beginning
 *                          ||
 *                          \/
 * ----------------------------------------
 * |   |   |   |   |   |   |   |   |
 * ----------------------------------------
 *
 * insert new block
 *                       Beginning
 *                          ||
 *                          \/
 * ------------------------------------
 * |   |   |   |   |   |   | 1200 |   |
 * ------------------------------------
 * 
 * 
 * insert new block which wraps around in the circular buffer
 *                         
 *                        Beginning
 *                            ||
 *                            \/
 * ------------------------------------------
 * | 1300 |   |   |   |   |   | 1200 | 1300 |
 * ------------------------------------------
 * 
 * Wrapping block will have 
 * 
 * struct block {
 *  int size = 412
 *  struct block next = the duplicate wrapped block if it is the first struct, else the nothing
 * } wrapping_block
 * 
 * This means that the algorithm have to check if something wraps
 * 
 * 
 * The open window (buffer space left) will be all the space from the last block to the beginning block
*/