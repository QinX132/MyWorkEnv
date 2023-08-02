#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleCommon.h"
#include "myMem.h"

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
AddHealthMonitor(
    StatReportCB Cb,
    int TimeIntervals
    );

#endif
