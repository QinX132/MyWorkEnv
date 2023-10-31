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
}
MY_TPOOL_MODULE_INIT_ARG;

int
TPoolModuleInit(
    MY_TPOOL_MODULE_INIT_ARG *InitArg
    );

void
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

#ifdef __cplusplus
}
#endif

#endif /* _MY_THREAD_POOL_H_ */
