#ifndef _MY_COMMON_UTIL_H_
#define _MY_COMMON_UTIL_H_
#include "include.h"

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

#endif
