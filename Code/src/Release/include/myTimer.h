#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myList.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _MY_TIMER_EVENT_NODE
{
    struct event* Event;
    MY_LIST_NODE List;
}
MY_TIMER_EVENT_NODE;

typedef MY_TIMER_EVENT_NODE* TIMER_HANDLE;

typedef enum _MY_TIMER_TYPE
{
    MY_TIMER_TYPE_ONE_TIME,
    MY_TIMER_TYPE_LOOP,
    
    MY_TIMER_TYPE_MAX_UNSPEC
}
MY_TIMER_TYPE;

typedef void (*TimerCB)(evutil_socket_t, short, void*);

void
TimerModuleExit(
    void
    );

int 
TimerModuleInit(
    void
    );

int
TimerAdd(
    TimerCB Cb,
    uint32_t IntervalMs,
    void* Arg,
    MY_TIMER_TYPE TimerType,
    __out TIMER_HANDLE *TimerHandle
    );
 
void
TimerDel(
    __inout TIMER_HANDLE *TimerHandle
    );

#ifdef __cplusplus
 }
#endif

#endif
