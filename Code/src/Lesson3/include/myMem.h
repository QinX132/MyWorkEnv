#ifndef _MY_MEM_H_
#define _MY_MEM_H_

#include "myModuleCommon.h"
#include "include.h"

/*  After the initialization process, you can utilize the "register" and "unregister" functions to 
 *  manage separate memids independently. Later, you can use "MemCalloc" and "MemFree" functions 
 *  for unified memid management. However, for the sake of convenience, I will handle the management 
 *  with a unified memid, using MyCalloc and MyFree.
 */

int
MemModuleInit(
    void
    );

void
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

void
MemUnRegister(
    int MemId
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

#endif
