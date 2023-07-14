#ifndef _MY_MODULE_COMMON_H_
#define _MY_MODULE_COMMON_H_

#include "include.h"

typedef enum _MY_MODULES_ENUM
{
    MY_MODULES_ENUM_LOG,
    MY_MODULES_ENUM_MSG,
    MY_MODULES_ENUM_TPOOL,
    MY_MODULES_ENUM_CMDLINE,
    MY_MODULES_ENUM_MHEALTH,

    MY_MODULES_ENUM_MASTER,
    MY_MODULES_ENUM_MAX
}
MY_MODULES_ENUM;

typedef struct _MY_MODULES_INIT_PARAM
{
    char* LogFile;
    char* RoleName;
    int TPoolSize;
    int TPoolTimeout;
    int Argc;
    char** Argv;
}
MY_MODULES_INIT_PARAM;

const char*
ModuleNameByEnum(
    int Module
    );

int
MyModuleCommonInit(
    MY_MODULES_INIT_PARAM ModuleInitParam
    );

void
MyModuleCommonExit(
    void
    );

#endif
