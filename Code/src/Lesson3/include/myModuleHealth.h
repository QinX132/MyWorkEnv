#ifndef _MY_MODULE_HEALTH_H_
#define _MY_MODULE_HEALTH_H_

#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleCommon.h"

typedef void (*StatReportCB)(evutil_socket_t, short, void*);

void
HealthModuleExit(
    void
    );

int 
HealthModuleInit(
    void
    );

#endif
