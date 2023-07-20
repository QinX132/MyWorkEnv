#include "myModuleHealth.h"

const StatReportCB sg_ModuleReprtCB[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   LogModuleCollectStat,
    [MY_MODULES_ENUM_MSG]       =   MsgModuleCollectStat,
    [MY_MODULES_ENUM_TPOOL]     =   TPoolModuleCollectStat,
    [MY_MODULES_ENUM_CMDLINE]   =   NULL,
    [MY_MODULES_ENUM_MHEALTH]   =   NULL
};

static int sg_ModuleReprtCBInterval[MY_MODULES_ENUM_MAX] = // seconds
{
    [MY_MODULES_ENUM_LOG]       =   30,
    [MY_MODULES_ENUM_MSG]       =   30,
    [MY_MODULES_ENUM_TPOOL]     =   30,
    [MY_MODULES_ENUM_CMDLINE]   =   0xff,
    [MY_MODULES_ENUM_MHEALTH]   =   0xff
};

static pthread_t * sg_HealthModuleT = NULL;
static BOOL sg_HeadlthModuleInited = FALSE;

static
void 
_HealthModule_SigHandler(
    int sig
    )
{
    if (sig == MY_TEST_KILL_SIGNAL)
        pthread_exit(NULL);
}

void
_HealthModuleStatCommonTemplate(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    char logBuff[MY_TEST_BUFF_256] = {0};
    int offset = 0;
    
    UNUSED(Arg);
    UNUSED(Fd);
    UNUSED(Event);

    StatReportCB cb = (StatReportCB)Arg;
    
    if (cb(logBuff, sizeof(logBuff), &offset) == 0)
    {
        LogInfo("%s", logBuff);
    }
    return ;
}

static void*
_HealthModule_Entry(
    void* Arg
    )
{
    struct event_base *base = NULL;
    struct timeval tv;
    struct event eventArr[MY_MODULES_ENUM_MAX];
    int loop = 0;
    
    UNUSED(Arg);
    signal(MY_TEST_KILL_SIGNAL, _HealthModule_SigHandler);

    base = event_base_new();
    if(NULL == base)
    {
        LogErr("New event base failed!\n");
        goto CommonReturn;
    }
    memset(eventArr, 0, sizeof(eventArr));
    for(loop = 0; loop < MY_MODULES_ENUM_MAX; loop ++)
    {
        if (sg_ModuleReprtCB[loop])
        {
            tv.tv_sec = sg_ModuleReprtCBInterval[loop];
            tv.tv_usec = 0;
            event_assign(&eventArr[loop], base, -1, EV_PERSIST, _HealthModuleStatCommonTemplate, (void*)sg_ModuleReprtCB[loop]);
            event_add(&eventArr[loop], &tv);
        }
    }

    event_base_dispatch(base);
    
CommonReturn:
    if(NULL != base)
    {
        event_base_free(base);
    }
    pthread_exit(NULL);
}

void
HealthModuleExit(
    void
    )
{
    if (sg_HeadlthModuleInited)
    {
        pthread_kill(*sg_HealthModuleT, MY_TEST_KILL_SIGNAL);
        free(sg_HealthModuleT);
        sg_HealthModuleT = NULL;
    }
}

int 
HealthModuleInit(
    void
    )
{
    int ret = 0;

    sg_HealthModuleT = (pthread_t*)malloc(sizeof(pthread_t));
    if (!sg_HealthModuleT)
    {
        ret = MY_EINVAL;
        LogErr("Apply memory failed!");
        goto CommonReturn;
    }
    
    pthread_create(sg_HealthModuleT, NULL, _HealthModule_Entry, NULL);
    sg_HeadlthModuleInited = TRUE;

CommonReturn:
    return ret;
}
