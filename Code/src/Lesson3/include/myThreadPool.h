#ifndef _MY_THREAD_POOL_H_
#define _MY_THREAD_POOL_H_

#include "include.h"
#define THREAD_POOL_SIZE                                5
#define TASK_QUEUE_SIZE                                 10

typedef struct _MY_TEST_THREAD_TASK
{
    void (*TaskFunc)(void*);
    void* TaskArg;
    BOOL TaskDone;
}
MY_TEST_THREAD_TASK;

typedef struct _MY_TEST_THREAD_POOL{
    pthread_mutex_t Lock;
    pthread_cond_t Cond;
    pthread_t *Threads;
    int ThreadNum;
    MY_TEST_THREAD_TASK TaskQueue[TASK_QUEUE_SIZE];
    int QueueFront;
    int QueueRear;
    int QueueCount;
    BOOL Exit;
}
MY_TEST_THREAD_POOL;

void
ThreadPoolModuleInit(
    MY_TEST_THREAD_POOL* ThreadPool,
    int ThreadPoolSize,
    int Timeout
    );

void
ThreadPoolModuleExit(
    MY_TEST_THREAD_POOL* ThreadPool
    );

int
AddTaskIntoThread(
    MY_TEST_THREAD_POOL* Thread_pool,
    void (*TaskFunc)(void*),
    void* TaskArg
    );

#endif /* _MY_THREAD_POOL_H_ */
