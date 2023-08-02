#ifndef _MY_MODULE_COMMON_H_
#define _MY_MODULE_COMMON_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myCmdLine.h"
#include "myModuleHealth.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _MY_MODULES_INIT_PARAM
{
    MY_CMDLINE_MODULE_INIT_ARG *CmdLineArg;
    MY_LOG_MODULE_INIT_ARG *LogArg;
    BOOL InitMsgModule;
    BOOL InitHealthModule;
    MY_TPOOL_MODULE_INIT_ARG *TPoolArg;
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

#ifdef __cplusplus
 }
#endif

#endif
