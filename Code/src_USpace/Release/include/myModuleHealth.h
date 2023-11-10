#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myMem.h"
#include "myTimer.h"
#include "myCmdLine.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_HEALTH_MONITOR_NAME_MAX_LEN                                      MY_BUFF_64

typedef enum _MY_MODULES_ENUM
{
    MY_MODULES_ENUM_LOG,
    MY_MODULES_ENUM_MSG,
    MY_MODULES_ENUM_TPOOL,
    MY_MODULES_ENUM_CMDLINE,
    MY_MODULES_ENUM_MHEALTH,
    MY_MODULES_ENUM_MEM,
    MY_MODULES_ENUM_TIMER,
    
    MY_MODULES_ENUM_MAX
}
MY_MODULES_ENUM;

typedef int (*StatReportCB)(char*, int, int*);

typedef struct _MODULE_HEALTH_REPORT_REGISTER
{
    StatReportCB Cb;
    int Interval;
}
MODULE_HEALTH_REPORT_REGISTER;

typedef struct _MY_HEALTH_MODULE_INIT_ARG
{
    int LogHealthIntervalS;
    int MsgHealthIntervalS;
    int TPoolHealthIntervalS;
    int CmdLineHealthIntervalS;
    int MHHealthIntervalS;
    int MemHealthIntervalS;
    int TimerHealthIntervalS;
}
MY_HEALTH_MODULE_INIT_ARG;

extern MODULE_HEALTH_REPORT_REGISTER sg_ModuleReprt[MY_MODULES_ENUM_MAX];

int
HealthModuleExit(
    void
    );

int 
HealthModuleInit(
    MY_HEALTH_MODULE_INIT_ARG *InitArg
    );

int
HealthMonitorAdd(
    StatReportCB Cb,
    const char* Name,
    int TimeIntervalS
    );

int
HealthModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );
 
const char*
ModuleNameByEnum(
    int Module
    );

#ifdef __cplusplus
 }
#endif

#endif
