#ifndef _MY_MEM_H_
#define _MY_MEM_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_MEM_MODULE_INVALID_ID                                -1

/*  After the initialization process, you can utilize the "register" and "unregister" functions to 
 *  manage separate memids independently. Later, you can use "MemCalloc" and "MemFree" functions 
 *  for unified memid management. However, for the sake of convenience, I will handle the management 
 *  with a unified memid, using MyCalloc and MyFree.
 */

int
MemModuleInit(
    void
    );

int
MemModuleExit(
    void
    );

int
MemModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

int
MemRegister(
    int *MemId,
    char *Name
    );

int
MemUnRegister(
    int* MemId
    );


void*
MemCalloc(
    int MemId,
    size_t Size
    );

void
MemFree(
    int MemId,
    void* Ptr
    );

void*
MyCalloc(
    size_t Size
    );

void
MyFree(
    void* Ptr
    );

BOOL
MemLeakSafetyCheck(
    void
    );

BOOL
MemLeakSafetyCheckWithId(
    int MemId
    );

#ifdef __cplusplus
 }
#endif

#endif
