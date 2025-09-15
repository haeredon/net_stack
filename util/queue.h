#ifndef UTIL_QUEUE_H
#define UTIL_QUEUE_H

#include<stdint.h>
#include<stdbool.h>
#include<pthread.h>
#include<rte_ring.h>

#define QUEUE_TYPE struct rte_ring
#define QUEUE_CREATE_QUEUE(SIZE) rte_ring_create("", SIZE, rte_socket_id(), RING_F_SP_ENQ) 
#define QUEUE_ENQUEUE(QUEUE, OBJ) rte_ring_enqueue(QUEUE, OBJ)
#define QUEUE_DEQUEUE(QUEUE, OBJ) rte_ring_dequeue(QUEUE, OBJ)

#endif // UTIL_QUEUE_H


 
