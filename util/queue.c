#include "util/queue.h"
#include "log.h"

#include<stdbool.h>


struct queue_t* queue_dequeue(struct queue_t* queue) {
    pthread_mutex_lock(&queue->lock);

    void* value = 0;
    if(queue->next == queue->head) {
        NETSTACK_LOG(NETSTACK_ERROR, "Thread work buffer full\n");   
    } else {
        value = queue->values[queue->head++];
    }

    pthread_mutex_unlock(&queue->lock);

    return value;
}

bool queue_enqueue(struct queue_t* queue, void* value) {
    pthread_mutex_lock(&queue->lock);

    if(queue->next == queue->head) {
        NETSTACK_LOG(NETSTACK_ERROR, "Thread work buffer full\n");   
        return false;
    } else {
        queue->values[queue->next++];
    }

    pthread_mutex_unlock(&queue->lock);
    
    return true;
}

struct queue_t* queue_create_queue(void* (*mem_allocate)(const char *type, size_t size)) {
    struct queue_t* queue = mem_allocate("Queue", sizeof(struct queue_t));

    queue->head = 0;
    queue->next = 0;    

    for(uint8_t i = 0 ; i < QUEUE_SIZE ; i++) {
        queue->values[i] = 0;
    }

    pthread_mutex_init(&queue->lock, 0);    

    return queue;
}


