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
ThreadPoolModuleInit(
    MY_TPOOL_MODULE_INIT_ARG *InitArg
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

void 
SetTPoolTimeout(
    uint32_t Timeout
    );

#ifdef __cplusplus
}
#endif

#endif /* _MY_THREAD_POOL_H_ */
