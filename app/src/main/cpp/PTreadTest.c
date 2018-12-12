//
// Created by Administrator on 2018-12-12.
//
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "queue.h"

#include "com_palyer_wz1_bhplayer_VideoPlayer.h"

struct testPthread
{
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    Queue * queue;
};

typedef struct testPthread testPth;

void pushInt(const Queue *pQueue, int value, pthread_mutex_t *mutex, pthread_cond_t *cond);

void *myThread1(void* arg)
{
    testPth *queue = (testPth *)arg;

    int i;
    for (int i = 0; i < 100; ++i) {
        printf("this is 1st myThread1.\n");
        pthread_mutex_lock(queue->mutex);
        pushInt(queue, i, queue->mutex, queue->cond);
        pthread_mutex_unlock(queue->mutex);
        sleep(1);
    }

    listQueue(queue->queue);

    return NULL;
}

void *myThread2(void* arg)
{
    testPth * queue = (testPth *)arg;
    int i;
    for (int i = 0; i < 100; ++i) {
        printf("this is 1st myThread2.\n");
        pthread_mutex_lock(queue->mutex);
        pushInt(queue, i, queue->mutex, queue->cond);
        pthread_mutex_unlock(queue->mutex);
        sleep(1);
    }

    return NULL;
}



JNIEXPORT void
pushInt(const Queue *pQueue, int value, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    pthread_mutex_lock(mutex);

    queuePush(pQueue,value,mutex,cond,1);

    pthread_mutex_unlock(mutex);
} void JNICALL Java_com_palyer_wz1_bhplayer_VideoPlayer_pThreadTest
        (JNIEnv * env, jobject jobject1){

    int ref;
    pthread_t pid1,pid2;
    testPth * test =(testPth*)malloc(sizeof(testPth));

    pthread_mutex_init(test->mutex,NULL);
    pthread_cond_init(test->cond,NULL);

    Queue * pQueue = createQueue();

    test->queue=pQueue;

   ref= pthread_create(&pid1, NULL, (void *) myThread1, (void*)test);
    if (ref)
    {
        printf("Pthread1 创建出错");
        return;
    }

    ref= pthread_create(&pid2,NULL,(void *)myThread2,(void*)test);
    if (ref)
    {
        printf("Pthread2 创建出错");
        return;
    }


    pthread_join(pid1,NULL);
    pthread_join(pid2,NULL);


    pthread_mutex_destroy(test->mutex);
    pthread_cond_destroy(test->cond);

    return;

}
