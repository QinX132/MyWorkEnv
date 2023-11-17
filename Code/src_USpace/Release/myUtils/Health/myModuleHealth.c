#include "myModuleHealth.h"
#include "myList.h"

#define MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL                          300 //s

MODULE_HEALTH_REPORT_REGISTER sg_ModuleReprt[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   {LogModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_MSG]       =   {MsgModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_TPOOL]     =   {TPoolModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_CMDLINE]   =   {CmdLineModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_MHEALTH]   =   {HealthModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_MEM]       =   {MemModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL},
    [MY_MODULES_ENUM_TIMER]     =   {TimerModuleCollectStat, MY_MODULE_HEALTH_DEFAULT_TIME_INTERVAL}
};

typedef struct _MY_HEALTH_MONITOR_LIST_NODE
{
    struct event* Event;
    char Name[MY_HEALTH_MONITOR_NAME_MAX_LEN];
    int32_t IntervalS;
    MY_LIST_NODE List;
}
MY_HEALTH_MONITOR_LIST_NODE;

typedef struct _MY_HEALTH_MONITOR
{
    pthread_t ThreadId;
    struct event_base* EventBase;
    BOOL IsRunning;
    MY_LIST_NODE EventList;     // MY_HEALTH_MONITOR_LIST_NODE
    int32_t EventListLen;
    pthread_spinlock_t Lock;
}
MY_HEALTH_MONITOR;

static MY_HEALTH_MONITOR sg_HealthWorker = {.IsRunning = FALSE};
static int32_t sg_HealthModId = MY_MEM_MODULE_INVALID_ID;

static void*
_HealthCalloc(
    size_t Size
    )
{
    return MemCalloc(sg_HealthModId, Size);
}

static void
_HealthFree(
    void* Ptr
    )
{
    return MemFree(sg_HealthModId, Ptr);
}

#define MY_HEALTH_MONITOR_KEEPALIVE_INTERVAL                1 // s
void
_HealthMonitorKeepalive(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    UNUSED(Fd);
    UNUSED(Event);
    UNUSED(Arg);
    return ;
}

void
_HealthModuleStatCommonTemplate(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    char logBuff[MY_BUFF_1024 * MY_BUFF_1024] = {0};
    int offset = 0;
    
    UNUSED(Arg);
    UNUSED(Fd);
    UNUSED(Event);

    StatReportCB cb = (StatReportCB)Arg;
    
    if (cb(logBuff, sizeof(logBuff), &offset) == 0 && strlen(logBuff))
    {
        LogDbg("%s", logBuff);
    }
    return ;
}

static void*
_HealthModuleEntry(
    void* Arg
    )
{
    struct timeval tv;
    int loop = 0;
    MY_HEALTH_MONITOR_LIST_NODE *node = NULL;
    
    UNUSED(Arg);

    sg_HealthWorker.EventBase = event_base_new();
    if(!sg_HealthWorker.EventBase)
    {
        LogErr("New event base failed!\n");
        goto CommonReturn;
    }
    // Set event base to be externally referencable and thread-safe
    if (evthread_make_base_notifiable(sg_HealthWorker.EventBase) < 0)
    {
        goto CommonReturn;
    }
    // keepalive
    node = (MY_HEALTH_MONITOR_LIST_NODE*)_HealthCalloc(sizeof(MY_HEALTH_MONITOR_LIST_NODE));
    if (!node)
    {
        goto CommonReturn;
    }
    node->Event = (struct event*)_HealthCalloc(sizeof(struct event));
    if (!node->Event)
    {
        goto CommonReturn;
    }
    tv.tv_sec = MY_HEALTH_MONITOR_KEEPALIVE_INTERVAL;
    tv.tv_usec = 0;
    snprintf(node->Name, sizeof(node->Name), "%s", "Keepalive");
    node->IntervalS = MY_HEALTH_MONITOR_KEEPALIVE_INTERVAL;
    event_assign(node->Event, sg_HealthWorker.EventBase, -1, EV_READ | EV_PERSIST, _HealthMonitorKeepalive, NULL);
    event_add(node->Event, &tv);
    event_active(node->Event, EV_READ, 0);
    MY_LIST_ADD_TAIL(&node->List, &sg_HealthWorker.EventList);
    sg_HealthWorker.EventListLen ++;
    node = NULL;
    
    for(loop = 0; loop < MY_MODULES_ENUM_MAX; loop ++)
    {
        if (sg_ModuleReprt[loop].Cb && sg_ModuleReprt[loop].Interval > 0)
        {
            node = (MY_HEALTH_MONITOR_LIST_NODE*)_HealthCalloc(sizeof(MY_HEALTH_MONITOR_LIST_NODE));
            if (!node)
            {
                goto CommonReturn;
            }
            node->Event = (struct event*)_HealthCalloc(sizeof(struct event));
            if (!node->Event)
            {
                goto CommonReturn;
            }
            snprintf(node->Name, sizeof(node->Name), "%s", ModuleNameByEnum(loop));
            node->IntervalS = sg_ModuleReprt[loop].Interval;
            tv.tv_sec = sg_ModuleReprt[loop].Interval;
            tv.tv_usec = 0;
            event_assign(node->Event, sg_HealthWorker.EventBase, -1, EV_PERSIST, _HealthModuleStatCommonTemplate, (void*)sg_ModuleReprt[loop].Cb);
            event_add(node->Event, &tv);
            MY_LIST_ADD_TAIL(&node->List, &sg_HealthWorker.EventList);
            sg_HealthWorker.EventListLen ++;
            node = NULL;
        }
    }

    sg_HealthWorker.IsRunning = TRUE;
    event_base_dispatch(sg_HealthWorker.EventBase);
    
CommonReturn:
    sg_HealthWorker.IsRunning = FALSE;
    if (node)
    {
        _HealthFree(node);
    }
    if (sg_HealthWorker.EventBase)
    {
        event_base_free(sg_HealthWorker.EventBase);
        sg_HealthWorker.EventBase = NULL;
    }
    pthread_exit(NULL);
}

int
HealthMonitorAdd(
    StatReportCB Cb,
    const char* Name,
    int TimeIntervalS
    )
{
    int ret = MY_SUCCESS;
    MY_HEALTH_MONITOR_LIST_NODE *node = NULL;
    struct timeval tv;
    
    node = (MY_HEALTH_MONITOR_LIST_NODE*)_HealthCalloc(sizeof(MY_HEALTH_MONITOR_LIST_NODE));
    if (!node)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    node->Event = (struct event*)_HealthCalloc(sizeof(struct event));
    if (!node->Event)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    tv.tv_sec = TimeIntervalS;
    tv.tv_usec = 0;
    if (Name)
    {
        snprintf(node->Name, sizeof(node->Name), "%s", Name);
    }
    node->IntervalS = TimeIntervalS;
    pthread_spin_lock(&sg_HealthWorker.Lock);
    event_assign(node->Event, sg_HealthWorker.EventBase, -1, EV_PERSIST, _HealthModuleStatCommonTemplate, (void*)Cb);
    event_add(node->Event, &tv);
    MY_LIST_ADD_TAIL(&node->List, &sg_HealthWorker.EventList);
    sg_HealthWorker.EventListLen ++;
    pthread_spin_unlock(&sg_HealthWorker.Lock);

CommonReturn:
    if (ret && node)
    {
        _HealthFree(node);
    }
    return ret;
}

int
HealthModuleExit(
    void
    )
{
    int ret = MY_SUCCESS;
    MY_HEALTH_MONITOR_LIST_NODE *tmp = NULL, *loop = NULL;
    
    if (sg_HealthWorker.IsRunning)
    {
        pthread_spin_lock(&sg_HealthWorker.Lock);
        sg_HealthWorker.EventListLen = 0;
        if (!MY_LIST_IS_EMPTY(&sg_HealthWorker.EventList))
        {
            MY_LIST_FOR_EACH(&sg_HealthWorker.EventList, loop, tmp, MY_HEALTH_MONITOR_LIST_NODE, List)
            {
                MY_LIST_DEL_NODE(&loop->List);
                event_del(loop->Event);
                //event_free(loop->Event);  // no need to free because we use event assign
                _HealthFree(loop->Event);
                _HealthFree(loop);
                loop = NULL;
            }
        }
        pthread_spin_unlock(&sg_HealthWorker.Lock);
        event_base_loopexit(sg_HealthWorker.EventBase, NULL);
        pthread_join(sg_HealthWorker.ThreadId, NULL);
        pthread_spin_destroy(&sg_HealthWorker.Lock);
        ret = MemUnRegister(&sg_HealthModId);
    }

    return ret;
}

int 
HealthModuleInit(
    MY_HEALTH_MODULE_INIT_ARG *InitArg
    )
{
    int ret = MY_SUCCESS;
    int sleepIntervalUs = 10;
    int waitTimeUs = sleepIntervalUs * 1000; // 10 ms

    if (sg_HealthWorker.IsRunning)
    {
        goto CommonReturn;
    }
    ret = MemRegister(&sg_HealthModId, "Health");
    if (ret)
    {
        LogErr("Register mem failed!\n");
        goto CommonReturn;
    }

    if (InitArg)
    {
        sg_ModuleReprt[MY_MODULES_ENUM_LOG].Interval = InitArg->LogHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_MSG].Interval = InitArg->MsgHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_TPOOL].Interval = InitArg->TPoolHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_CMDLINE].Interval = InitArg->CmdLineHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_MHEALTH].Interval = InitArg->MHHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_MEM].Interval = InitArg->MemHealthIntervalS;
        sg_ModuleReprt[MY_MODULES_ENUM_TIMER].Interval = InitArg->TimerHealthIntervalS;
    }
    
    pthread_spin_init(&sg_HealthWorker.Lock, PTHREAD_PROCESS_PRIVATE);
    MY_LIST_NODE_INIT(&sg_HealthWorker.EventList);
    sg_HealthWorker.EventListLen = 0;
    ret = pthread_create(&sg_HealthWorker.ThreadId, NULL, _HealthModuleEntry, NULL);
    if (ret)
    {
        goto CommonReturn;
    }

    while(!sg_HealthWorker.IsRunning && waitTimeUs >= 0)
    {
        usleep(sleepIntervalUs);
        waitTimeUs -= sleepIntervalUs;
    }

CommonReturn:
    return ret;
}

int
HealthModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = MY_SUCCESS;
    MY_HEALTH_MONITOR_LIST_NODE *tmp = NULL, *loop = NULL;
    int len = 0;

    len += snprintf(Buff + *Offset + len, BuffMaxLen - *Offset - len, 
            "<%s:(ListLength:%d)", ModuleNameByEnum(MY_MODULES_ENUM_MHEALTH), sg_HealthWorker.EventListLen);
    if (!MY_LIST_IS_EMPTY(&sg_HealthWorker.EventList))
    {
        MY_LIST_FOR_EACH(&sg_HealthWorker.EventList, loop, tmp, MY_HEALTH_MONITOR_LIST_NODE, List)
        {
            len += snprintf(Buff + *Offset + len, BuffMaxLen - *Offset - len, 
                "[EventName:%s, EventInterval:%d]", loop->Name, loop->IntervalS);
            if (len < 0 || len >= BuffMaxLen - *Offset - len)
            {
                ret = -MY_ENOMEM;
                LogErr("Too long Msg!");
                goto CommonReturn;
            }
        }
        
    }
    
    len += snprintf(Buff + *Offset + len, BuffMaxLen - *Offset - len, ">");
    
CommonReturn:
    *Offset += len;
    return ret;
}

static const char* sg_ModulesName[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   "Log",
    [MY_MODULES_ENUM_MSG]       =   "Msg",
    [MY_MODULES_ENUM_TPOOL]     =   "TPool",
    [MY_MODULES_ENUM_CMDLINE]   =   "CmdLine",
    [MY_MODULES_ENUM_MHEALTH]   =   "MHealth",
    [MY_MODULES_ENUM_MEM]       =   "Mem",
    [MY_MODULES_ENUM_TIMER]     =   "Timer"
};

const char*
ModuleNameByEnum(
    int Module
    )
{
    return (Module >= 0 && Module < (int)MY_MODULES_ENUM_MAX) ? sg_ModulesName[Module] : NULL;
}

