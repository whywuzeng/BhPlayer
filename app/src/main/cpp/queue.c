//
// Created by Administrator on 2018-12-12.
//

#include "queue.h"
#include "android/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

//log设置
#define LOG_TAG "queue.c"

#define LOG_ERR(FORMAT,...)   __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,FORMAT,__VA_ARGS__)
#define ERR(String)  LOG_ERR("%s",String)

//大小
#define MAX_SIZE 15
#define ERRORTYPE 99;

//结构体属性
//0.1头 尾 总
struct _Queue{
    ElementType data[MAX_SIZE];
    int front;
    int rear;
    int size;
};

//建
    //初始化
Queue* createQueue(){
    Queue* queue=(Queue*)malloc(sizeof(Queue*));
    if (!queue)
    {
        printf("queue分配的内存空间不足");
        return NULL;
    }
    queue->size=0;
    queue->front=-1;
    queue->rear=-1;
    return queue;
}

//是否已满
int isFull(Queue* q){
    return q->size == MAX_SIZE;
}

int isEmpty(Queue * q)
{
    return q->size ==0;
}

//push压入
    //数值  相应的变化
    //用互斥锁 mutex  和 条件cond
void queuePush(Queue* queue,ElementType item,pthread_mutex_t *mutex,pthread_cond_t *cond,int abort_request){
    if (abort_request)
    {
        ERR("abort_request");
        return;
    }

    if (!isFull(queue))
    {
        queue->rear++;
        queue->rear %=MAX_SIZE;
        queue->size++;
        queue->data[queue->rear]=item;
        //通知
        pthread_cond_broadcast(cond);
    } else{
        ERR("队列已满");
        pthread_cond_wait(cond,mutex);
    }

  return;
}

//取出
  ElementType QueuePop(Queue* queue,pthread_mutex_t *mutex,pthread_cond_t *cond,int abort_request)
{
    if (abort_request)
    {
        ERR("abort_request");
        ElementType ele;
        return ele;
    }

    while (true){
        if (!isEmpty(queue))
        {
            queue->front++;
            queue->front%=MAX_SIZE;
            queue->size--;
            //通知
            pthread_cond_broadcast(cond);
            return queue->data[queue->front];
        } else{
            pthread_cond_wait(cond,mutex);
        }
    }
}

//销毁队列
void freeQueue(Queue* queue){
    free(queue->data);
    free(queue);
}

void listQueue(Queue* queue)
{
    int lenght= sizeof(queue->data)/ sizeof(ElementType);
    LOG_ERR("queue front的为:%d",queue->front);
    LOG_ERR("queue rear的为:%d",queue->rear);
    LOG_ERR("queue size的为:%d",queue->size);

    LOG_ERR("queue数组的长度为:%d",lenght);
    for (int i = 0; i < lenght; ++i) {
        printf("queue里的index:%d,value:%d",i,queue->data[i]);
    }
}