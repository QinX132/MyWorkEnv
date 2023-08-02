#include "myModuleHealth.h"
#include "myList.h"

typedef struct _HEALTH_MODULE_EVENT_NODE
{
    struct timeval Tv;
    void* Cb;
    MY_LIST_NODE ListNode;
}
HEALTH_MODULE_EVENT_NODE;

const MODULE_HEALTH_REPORT_REGISTER sg_ModuleReprt[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   {LogModuleCollectStat, 30},
    [MY_MODULES_ENUM_MSG]       =   {MsgModuleCollectStat, 30},
    [MY_MODULES_ENUM_TPOOL]     =   {TPoolModuleCollectStat, 30},
    [MY_MODULES_ENUM_CMDLINE]   =   {NULL, 0xff},
    [MY_MODULES_ENUM_MHEALTH]   =   {NULL, 0xff},
    [MY_MODULES_ENUM_MEM]       =   {MemModuleCollectStat, 30}
};

static pthread_t *sg_HealthModuleT = NULL;
static struct event_base *sg_HealthEventBase = NULL;
static MY_LIST_NODE sg_HealthListHead;
static pthread_spinlock_t sg_HealthSpinlock;

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

void
_HealthWorkerListenerCb(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    UNUSED(Arg);
    UNUSED(Fd);
    UNUSED(Event);

    HEALTH_MODULE_EVENT_NODE *tmpNode, *loop;
    struct event *eventTmp;

    pthread_spin_lock(&sg_HealthSpinlock);
    
    if (MY_LIST_IS_EMPTY(&sg_HealthListHead))
    {
        goto CommonReturn;
    }
    
    MY_LIST_ALL(&sg_HealthListHead, loop, tmpNode, HEALTH_MODULE_EVENT_NODE, ListNode)
    {
        eventTmp = (struct event*)MyCalloc(sizeof(struct event));
        if (!eventTmp)
        {
            goto CommonReturn;
        }
        event_assign(eventTmp, sg_HealthEventBase, -1, EV_PERSIST, _HealthModuleStatCommonTemplate, loop->Cb);
        event_add(eventTmp, &loop->Tv);
        MY_LIST_DEL_NODE(&loop->ListNode, &sg_HealthListHead);
        MyFree(loop);
        loop = NULL;
    }

CommonReturn:
    pthread_spin_unlock(&sg_HealthSpinlock);
    return ;
}

static void*
_HealthModule_Entry(
    void* Arg
    )
{
    struct timeval tv;
    struct event eventArr[MY_MODULES_ENUM_MAX];
    struct event eventTmp;
    int loop = 0;
    
    UNUSED(Arg);

    sg_HealthEventBase = event_base_new();
    if(!sg_HealthEventBase)
    {
        LogErr("New event base failed!\n");
        goto CommonReturn;
    }
    
    memset(eventArr, 0, sizeof(eventArr));
    for(loop = 0; loop < MY_MODULES_ENUM_MAX; loop ++)
    {
        if (sg_ModuleReprt[loop].Cb)
        {
            tv.tv_sec = sg_ModuleReprt[loop].Interval;
            tv.tv_usec = 0;
            event_assign(&eventArr[loop], sg_HealthEventBase, -1, EV_PERSIST, _HealthModuleStatCommonTemplate, (void*)sg_ModuleReprt[loop].Cb);
            event_add(&eventArr[loop], &tv);
        }
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_assign(&eventTmp, sg_HealthEventBase, -1, EV_PERSIST, _HealthWorkerListenerCb, NULL);
    event_add(&eventTmp, &tv);

    event_base_dispatch(sg_HealthEventBase);
    
CommonReturn:
    pthread_exit(NULL);
}

int
AddHealthMonitor(
    StatReportCB Cb,
    int TimeIntervals
    )
{
    int ret = 0;
    BOOL locked = FALSE;

    if (!sg_HealthEventBase)
    {
        ret = MY_EINVAL;
        LogErr("Headlth module uninited!");
        goto CommonReturn;
    }
    if (!Cb || TimeIntervals <= 0)
    {
        ret = MY_EINVAL;
        LogErr("Invalid input!");
        goto CommonReturn;
    }
    
    pthread_spin_lock(&sg_HealthSpinlock);
    locked = TRUE;
    {
        HEALTH_MODULE_EVENT_NODE *eventNode = NULL;
        eventNode = (HEALTH_MODULE_EVENT_NODE *)MyCalloc(sizeof( HEALTH_MODULE_EVENT_NODE));
        if (!eventNode)
        {
            ret = MY_ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        MY_LIST_NODE_INIT(&eventNode->ListNode);
        eventNode->Tv.tv_sec = TimeIntervals;
        eventNode->Tv.tv_usec = 0;
        eventNode->Cb = (void*)Cb;
        MY_LIST_ADD_TAIL(&eventNode->ListNode, &sg_HealthListHead);
    }
    pthread_spin_unlock(&sg_HealthSpinlock);
    locked = FALSE;

CommonReturn:
    if (locked)
    {
        pthread_spin_unlock(&sg_HealthSpinlock);
    }
    return ret;
}

void
HealthModuleExit(
    void
    )
{
    HEALTH_MODULE_EVENT_NODE *tmpNode, *loop;
    
    if (sg_HealthEventBase)
    {
        event_base_loopexit(sg_HealthEventBase, NULL);
        pthread_join(*sg_HealthModuleT, NULL);
        event_base_free(sg_HealthEventBase);
        MyFree(sg_HealthModuleT);
        sg_HealthModuleT = NULL;
        sg_HealthEventBase = NULL;
        pthread_spin_destroy(&sg_HealthSpinlock);
        if (!MY_LIST_IS_EMPTY(&sg_HealthListHead))
        {
            MY_LIST_ALL(&sg_HealthListHead, loop, tmpNode, HEALTH_MODULE_EVENT_NODE, ListNode)
            {
                MY_LIST_DEL_NODE(&loop->ListNode, &sg_HealthListHead);
                MyFree(loop);
                loop = NULL;
            }
        }
    }
}

int 
HealthModuleInit(
    void
    )
{
    int ret = 0;

    sg_HealthModuleT = (pthread_t*)MyCalloc(sizeof(pthread_t));
    if (!sg_HealthModuleT)
    {
        ret = MY_EINVAL;
        LogErr("Apply memory failed!");
        goto CommonReturn;
    }
    pthread_spin_init(&sg_HealthSpinlock, PTHREAD_PROCESS_PRIVATE);
    MY_LIST_NODE_INIT(&sg_HealthListHead);
    pthread_create(sg_HealthModuleT, NULL, _HealthModule_Entry, NULL);

CommonReturn:
    return ret;
}
