#ifndef _MY_THREAD_POOL_H_
#define _MY_THREAD_POOL_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _MY_TPOOL_MODULE_INIT_ARG
{
    int ThreadPoolSize;
    int Timeout;
    int TaskListMaxLength;
}
MY_TPOOL_MODULE_INIT_ARG;

int
TPoolModuleInit(
    MY_TPOOL_MODULE_INIT_ARG *InitArg
    );

int
TPoolModuleExit(
    void
    );

int
TPoolAddTask(
    void (*TaskFunc)(void*),
    void* TaskArg
    );

int
TPoolAddTaskAndWait(
    void (*TaskFunc)(void*),
    void* TaskArg,
    int32_t TimeoutSec
    );

int
TPoolModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

void 
TPoolSetTimeout(
    uint32_t Timeout
    );

void
TPoolSetMaxQueueLength(
    int32_t QueueLen
    );

#ifdef __cplusplus
}
#endif

#endif /* _MY_THREAD_POOL_H_ */
