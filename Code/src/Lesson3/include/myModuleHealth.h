#ifndef _MY_MODULE_COMMON_H_
#define _MY_MODULE_COMMON_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"

typedef enum _MY_MODULES_ENUM
{
    MY_MODULES_ENUM_LOG,
    MY_MODULES_ENUM_MSG,
    MY_MODULES_ENUM_TPOOL,
    MY_MODULES_ENUM_CMDLINE,
    
    MY_MODULES_ENUM_MAX
}MY_MODULES_ENUM;

typedef void (*StatReportCB)(evutil_socket_t, short, void*);

const char*
HealthModuleNameByEnum(
    int Module
    );

StatReportCB
HealthModuleStatCbByEnum(
    int Module
    );

void
HealthModuleExit(
    void
    );

int 
HealthModuleInit(
    void
    );

#endif
