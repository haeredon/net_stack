#ifndef UTIL_QUEUE_H
#define UTIL_QUEUE_H

#include<stdint.h>
#include<stdbool.h>
#include<pthread.h>


#define QUEUE_SIZE 64

struct queue_t {
    void* values[QUEUE_SIZE];
    uint8_t next;
    uint8_t head;
    pthread_mutex_t lock;
};


struct queue_t* queue_create_queue();
void* queue_dequeue(struct queue_t* queue);
bool queue_enqueue(struct queue_t* queue, void* value);

#endif // UTIL_QUEUE_H


 
