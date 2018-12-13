//
// Created by Administrator on 2018-12-12.
//

#ifndef BHPLAYER_QUEUE_H
#define BHPLAYER_QUEUE_H

#include <libavcodec/avcodec.h>

#define ElementType AVPacket

#define true 1
#define false 0

typedef struct _Queue Queue;

Queue* createQueue();

void queuePush(Queue* queue,ElementType item,pthread_mutex_t *mutex,pthread_cond_t *cond,int abort_request);

ElementType QueuePop(Queue* queue,pthread_mutex_t *mutex,pthread_cond_t *cond,int abort_request);

void listQueue(Queue* queue);

void freeQueue(Queue* queue);

#endif //BHPLAYER_QUEUE_H
