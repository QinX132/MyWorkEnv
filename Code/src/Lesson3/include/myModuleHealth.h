#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleCommon.h"

typedef int (*StatReportCB)(char*, int, int*);

extern const StatReportCB sg_ModuleReprtCB[MY_MODULES_ENUM_MAX];

void
HealthModuleExit(
    void
    );

int 
HealthModuleInit(
    void
    );

#endif
