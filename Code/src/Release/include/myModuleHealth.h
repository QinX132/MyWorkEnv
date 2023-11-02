#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myMem.h"
#include "myTimer.h"

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

typedef struct
{
    StatReportCB Cb;
    int Interval;
}
MODULE_HEALTH_REPORT_REGISTER;

extern const MODULE_HEALTH_REPORT_REGISTER sg_ModuleReprt[MY_MODULES_ENUM_MAX];

void
HealthModuleExit(
    void
    );

int 
HealthModuleInit(
    void
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

#ifdef __cplusplus
 }
#endif

#endif
