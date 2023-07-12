#ifndef _MY_MODULE_COMMON_H_
#define _MY_MODULE_COMMON_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"

typedef enum _MY_MODULES_ENUM
{
    MY_MODULES_ENUM_LOG,
    MY_MODULES_ENUM_MSG,
    MY_MODULES_ENUM_TPOOL,
    MY_MODULES_ENUM_CMDLINE,
    MY_MODULES_ENUM_MHEALTH,
    
    MY_MODULES_ENUM_MAX
}
MY_MODULES_ENUM;

const char*
ModuleNameByEnum(
    int Module
    );

int
MyModuleCommonInit(
    char* LogFile,
    char* RoleName,
    int TPoolSize,
    int TPoolTimeout
    );

void
MyModuleCommonExit(
    void
    );

#endif
