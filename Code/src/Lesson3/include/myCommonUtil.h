#ifndef _MY_COMMON_UTIL_H_
#define _MY_COMMON_UTIL_H_
#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_UTIL_GET_CPU_USAGE_START                     \
    do { \
        uint64_t _MY_UTIL_TOTAL_CPU_START = 0, _MY_UTIL_TOTAL_CPU_END = 0; \
        uint64_t _MY_UTIL_IDLE_CPU_START = 0, _MY_UTIL_IDLE_CPU_END = 0; \
        (void)MyUtil_GetCpuTime(&_MY_UTIL_TOTAL_CPU_START, &_MY_UTIL_IDLE_CPU_START);

#define MY_UTIL_GET_CPU_USAGE_END(_CPU_USAGE_)                       \
        (void)MyUtil_GetCpuTime(&_MY_UTIL_TOTAL_CPU_END, &_MY_UTIL_IDLE_CPU_END); \
        _CPU_USAGE_ = _MY_UTIL_TOTAL_CPU_END > _MY_UTIL_TOTAL_CPU_START ? \
                    1.0 - (double)(_MY_UTIL_IDLE_CPU_END - _MY_UTIL_IDLE_CPU_START) / \
                                (_MY_UTIL_TOTAL_CPU_END - _MY_UTIL_TOTAL_CPU_START) : 0; \
    }while(0);

void
MyUtil_MakeDaemon(
    void
    );

int
MyUtil_OpenPidFile(
    char* Path
    );

int
MyUtil_IsProcessRunning(
    int Fd
    );

int
MyUtil_SetPidIntoFile(
    int Fd
    );

void 
MyUtil_CloseStdFds(
    void
    );

int
MyUtil_GetCpuTime(
    uint64_t *TotalTime,
    uint64_t *IdleTime
    );

#ifdef __cplusplus
 }
#endif

#endif
