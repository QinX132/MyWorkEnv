#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myLogIO.h"
#include "myList.h"
#include "assert.h"
#include "myCommonUtil.h"

#define MY_TASK_TIME_OUT_DEFAULT_VAL                            5 //seconds
#define THREAD_POOL_SIZE                                        5

typedef enum _MY_TPOOL_TASK_STATUS
{
    MY_TPOOL_TASK_STATUS_UNSPEC,
    MY_TPOOL_TASK_STATUS_INIT,
    MY_TPOOL_TASK_STATUS_WORKING,
    MY_TPOOL_TASK_STATUS_TIMEOUT
}
MY_TPOOL_TASK_STATUS;

typedef struct _MY_TPOOL_TASK
{
    void (*TaskFunc)(void*);
    void* TaskArg;
    BOOL HasTimeOut;
    pthread_mutex_t *TaskLock;
    pthread_cond_t *TaskCond;
    MY_TPOOL_TASK_STATUS TaskStat;
    MY_LIST_NODE List;
}
MY_TPOOL_TASK;

typedef struct _MY_THREAD_POOL{
    pthread_mutex_t Lock;
    pthread_cond_t Cond;
    pthread_t *Threads;
    volatile int CurrentThreadNum;
    MY_LIST_NODE TaskListHead;      // MY_TEST_THREAD_TASK
    int TaskListLength;
    volatile BOOL Exit;
}
MY_THREAD_POOL;

//__thread 
MY_THREAD_POOL* sg_ThreadPool = NULL;
//__thread 
int sg_TPoolTaskTimeout = MY_TASK_TIME_OUT_DEFAULT_VAL;
//__thread 
BOOL sg_TPoolModuleInited = FALSE;

static int sg_TPoolTaskAdded = 0;
static int sg_TPoolTaskSucceed = 0;
static int sg_TPoolTaskFailed = 0;

static void* 
_TPoolFunction(
    void* arg
    )
{
    MY_THREAD_POOL* threadPool = (MY_THREAD_POOL*)arg;
    MY_TPOOL_TASK* loop = NULL, *tmp = NULL;
    MY_LIST_NODE listHeadTmp;
    LogInfo("Thread worker %lu entering...", pthread_self());
    
    while (!threadPool->Exit) 
    {
        pthread_mutex_lock(&threadPool->Lock);
        MY_UATOMIC_INC(&threadPool->CurrentThreadNum);
        while(pthread_cond_wait(&threadPool->Cond, &threadPool->Lock) == 0)
        {
            if (threadPool->Exit)
            {
                pthread_mutex_unlock(&threadPool->Lock);
                goto CommonReturn;
            }
            else
            {
                MY_LIST_HEAD_COPY(&listHeadTmp, &threadPool->TaskListHead);
                MY_LIST_NODE_INIT(&threadPool->TaskListHead);
                threadPool->TaskListLength = 0;
                break;
            }
        };
        MY_UATOMIC_DEC(&threadPool->CurrentThreadNum);
        pthread_mutex_unlock(&threadPool->Lock);

        MY_LIST_FOR_EACH(&listHeadTmp, loop, tmp, MY_TPOOL_TASK, List)
        {
            if (loop && loop->TaskFunc)
            {
                if (loop->HasTimeOut && loop->TaskStat == MY_TPOOL_TASK_STATUS_TIMEOUT)
                {
                    pthread_mutex_destroy(loop->TaskLock);
                    pthread_cond_destroy(loop->TaskCond);
                    MyFree(loop->TaskLock);
                    MyFree(loop->TaskCond);
                }
                else
                {
                    loop->TaskFunc(loop->TaskArg);
                    if (loop->HasTimeOut)
                    {
                        pthread_mutex_lock(loop->TaskLock);
                        pthread_cond_signal(loop->TaskCond);
                        pthread_mutex_unlock(loop->TaskLock);
                    }
                    MY_UATOMIC_INC(&sg_TPoolTaskSucceed);
                }
                MY_LIST_DEL_NODE(&loop->List);
                MyFree(loop);
                loop = NULL;
            }
        }
    }

CommonReturn:
    MY_UATOMIC_DEC(&threadPool->CurrentThreadNum);
    LogInfo("Thread worker %lu exit.", pthread_self());
    pthread_exit(NULL);
}

int
TPoolModuleInit(
    MY_TPOOL_MODULE_INIT_ARG *InitArg
    )
{
    int loop = 0;
    int ret = 0;
    pthread_attr_t attr;

    if (sg_TPoolModuleInited)
    {
        goto CommonReturn;
    }

    if (!InitArg || !InitArg->ThreadPoolSize)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    sg_ThreadPool = (MY_THREAD_POOL*)MyCalloc(sizeof(MY_THREAD_POOL));
    if (!sg_ThreadPool)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }
    
    pthread_mutex_init(&sg_ThreadPool->Lock, NULL);
    pthread_cond_init(&sg_ThreadPool->Cond, NULL);
    sg_ThreadPool->Exit = FALSE;
    sg_ThreadPool->CurrentThreadNum = 0;
    MY_LIST_NODE_INIT(&sg_ThreadPool->TaskListHead);
    sg_ThreadPool->TaskListLength = 0;
    
    ret = pthread_attr_init(&attr);
    assert(!ret);
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    assert(!ret);

    sg_ThreadPool->Threads = (pthread_t*)MyCalloc(sizeof(pthread_t) * InitArg->ThreadPoolSize);
    for (loop = 0; loop < InitArg->ThreadPoolSize; loop ++) {
        ret = pthread_create(&(sg_ThreadPool->Threads[loop]), &attr, _TPoolFunction, (void*)sg_ThreadPool);
        assert(!ret);
    }
    ret = pthread_attr_destroy(&attr);
    assert(!ret);

    sg_TPoolTaskTimeout = InitArg->Timeout ? InitArg->Timeout : MY_TASK_TIME_OUT_DEFAULT_VAL;
    sg_TPoolModuleInited = TRUE;

    // to make sure workers are ready
    while(1)
    {
        pthread_mutex_lock(&sg_ThreadPool->Lock);
        if (sg_ThreadPool->CurrentThreadNum >= InitArg->ThreadPoolSize)
        {
            pthread_mutex_unlock(&sg_ThreadPool->Lock);
            break;
        }
        pthread_mutex_unlock(&sg_ThreadPool->Lock);
        usleep(10 * 1000);
    };
    
CommonReturn:
    return ret;
}

void
TPoolModuleExit(
    void
    )
{
    int loop = 0;

    if (sg_TPoolModuleInited)
    {
        sg_ThreadPool->Exit = TRUE;
        pthread_mutex_lock(&(sg_ThreadPool->Lock));
        pthread_cond_broadcast(&(sg_ThreadPool->Cond));
        pthread_mutex_unlock(&(sg_ThreadPool->Lock));

        for (loop = 0; loop < sg_ThreadPool->CurrentThreadNum; loop++) {
            pthread_cond_broadcast(&(sg_ThreadPool->Cond));
        }

        pthread_mutex_destroy(&(sg_ThreadPool->Lock));
        pthread_cond_destroy(&(sg_ThreadPool->Cond));
        MyFree(sg_ThreadPool->Threads);
        MyFree(sg_ThreadPool);
        sg_ThreadPool = NULL;
        sg_TPoolModuleInited = FALSE;
    }
}

// async api, TaskArg is a in arg
int
TPoolAddTask(
    void (*TaskFunc)(void*),
    __in void* TaskArg
    )
{
    int ret = 0;
    MY_TPOOL_TASK *node = NULL;
    
    if (!sg_TPoolModuleInited)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    node = (MY_TPOOL_TASK*)MyCalloc(sizeof(MY_TPOOL_TASK));
    if (!node) 
    {
        ret = MY_ENOMEM;
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    node->HasTimeOut = FALSE;
    node->TaskArg = TaskArg;
    node->TaskFunc = TaskFunc;
    MY_LIST_NODE_INIT(&node->List);
    
    pthread_mutex_lock(&sg_ThreadPool->Lock);
    {
        MY_LIST_ADD_TAIL(&node->List, &sg_ThreadPool->TaskListHead);
        sg_ThreadPool->TaskListLength++;
        pthread_cond_signal(&sg_ThreadPool->Cond);
        MY_UATOMIC_INC(&sg_TPoolTaskAdded);
        LogInfo("Add task into tail success.");
    }
    pthread_mutex_unlock(&sg_ThreadPool->Lock);
    
CommonReturn:
    return ret;
}

// sync api, you can care about TaskArg inout
// when TimeoutSec > 0, use it; otherwise use sg_TPoolTaskTimeout
int
TPoolAddTaskAndWait(
    void (*TaskFunc)(void*),
    __inout void* TaskArg,
    int32_t TimeoutSec
    )
{
    int ret = 0;
    struct timespec ts = {0, 0};
    BOOL taskAdded = FALSE;
    pthread_mutex_t *taskLock;
    pthread_cond_t *taskCond;
    pthread_condattr_t taskAttr;
    BOOL taskInited = FALSE;
    MY_TPOOL_TASK *node = NULL;
    
    if (!sg_TPoolModuleInited || (sg_TPoolTaskTimeout <= 0 && TimeoutSec <= 0))
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    taskLock = (pthread_mutex_t*)MyCalloc(sizeof(pthread_mutex_t));
    taskCond = (pthread_cond_t*)MyCalloc(sizeof(pthread_cond_t));
    node = (MY_TPOOL_TASK*)MyCalloc(sizeof(MY_TPOOL_TASK));
    if (!taskLock || !taskCond || !node)
    {
        ret = MY_ENOMEM;
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    
    pthread_condattr_init(&taskAttr);
    pthread_condattr_setclock(&taskAttr, CLOCK_MONOTONIC);
    pthread_mutex_init(taskLock, NULL);
    pthread_cond_init(taskCond, &taskAttr);
    pthread_condattr_destroy(&taskAttr);
    taskInited = TRUE;
    
    node->HasTimeOut = TRUE;
    node->TaskArg = TaskArg;
    node->TaskFunc = TaskFunc;
    node->TaskCond = taskCond;
    node->TaskLock = taskLock;
    node->TaskStat = MY_TPOOL_TASK_STATUS_INIT;
    
    pthread_mutex_lock(&sg_ThreadPool->Lock);
    {
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == MY_SUCCESS)
        {
            MY_LIST_ADD_TAIL(&node->List, &sg_ThreadPool->TaskListHead);
            sg_ThreadPool->TaskListLength ++;
            taskAdded = TRUE;
            pthread_mutex_lock(taskLock);
            pthread_cond_signal(&sg_ThreadPool->Cond);
            MY_UATOMIC_INC(&sg_TPoolTaskAdded);
        }
        else
        {
            ret = MY_EIO;
            LogErr("Get time failed!");
        }
    }
    pthread_mutex_unlock(&sg_ThreadPool->Lock);

    if (taskAdded)
    {
        if (TimeoutSec > 0)
        {
            ts.tv_sec += TimeoutSec;
            LogInfo("Add with timeout %d", TimeoutSec);
        }
        else
        {
            ts.tv_sec += sg_TPoolTaskTimeout;
            LogInfo("Add with timeout %d", sg_TPoolTaskTimeout);
        }
        ret = pthread_cond_timedwait(taskCond, taskLock, &ts);
    }
    if (ETIMEDOUT == ret)
    {
        LogErr("Task wait timeout!");
        node->TaskStat = MY_TPOOL_TASK_STATUS_TIMEOUT;
    }
    pthread_mutex_unlock(taskLock);
    
CommonReturn:
    ret == MY_SUCCESS ? UNUSED(ret) : MY_UATOMIC_INC(&sg_TPoolTaskFailed);
    if (taskInited)
    {
        if (!taskAdded || ret != ETIMEDOUT)
        {
            pthread_mutex_destroy(taskLock);
            pthread_cond_destroy(taskCond);
            MyFree(taskLock);
            MyFree(taskCond);
        }
    }
    return ret;
}

int
TPoolModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = 0;
    int len = 0;
    
    if (!sg_TPoolModuleInited)
    {
        // protect sg_ThreadPool
        goto CommonReturn;
    }
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
        "<%s:[TaskAdded=%d, TaskSucceed=%d, TaskFailed=%d], CurrentThreadNum=%d, TaskListLength=%d>",
            ModuleNameByEnum(MY_MODULES_ENUM_TPOOL), sg_TPoolTaskAdded, sg_TPoolTaskSucceed, sg_TPoolTaskFailed,
            sg_ThreadPool->CurrentThreadNum, sg_ThreadPool->TaskListLength);
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }
CommonReturn:
    return ret;
}

void
TPoolSetTimeout(
    uint32_t Timeout
    )
{
    sg_TPoolTaskTimeout = Timeout;
}
