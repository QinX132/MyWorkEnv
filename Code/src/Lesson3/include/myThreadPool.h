#ifndef _MY_THREAD_POOL_H_
#define _MY_THREAD_POOL_H_

#include "include.h"
#include "myModuleCommon.h"
#define THREAD_POOL_SIZE                                5
#define TASK_QUEUE_SIZE                                 64

typedef struct _MY_TEST_THREAD_TASK
{
    void (*TaskFunc)(void*);
    void* TaskArg;
    BOOL HasTimeOut;
    pthread_mutex_t *TaskLock;
    pthread_cond_t *TaskCond;
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

int
ThreadPoolModuleInit(
    int ThreadPoolSize,
    int Timeout
    );

void
ThreadPoolModuleExit(
    void
    );

int
AddTaskIntoThread(
    void (*TaskFunc)(void*),
    void* TaskArg
    );

int
AddTaskIntoThreadAndWait(
    void (*TaskFunc)(void*),
    void* TaskArg
    );

int
TPoolModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

#endif /* _MY_THREAD_POOL_H_ */
