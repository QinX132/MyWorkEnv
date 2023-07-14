#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myLogIO.h"

#define MY_TEST_TASK_TIME_OUT_DEFAULT_VAL                       5 //seconds
//__thread 
MY_TEST_THREAD_POOL* sg_ThreadPool = NULL;
//__thread 
int sg_ThreadPoolTaskTimeout = MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
//__thread 
BOOL sg_TPoolModuleInited = FALSE;

static int sg_TPoolTaskAdded = 0;
static int sg_TPoolTaskSucceed = 0;
static int sg_TPoolTaskFailed = 0;

static void* 
_ThreadPoolFunction(
    void* arg
    )
{
    MY_TEST_THREAD_POOL* thread_pool = (MY_TEST_THREAD_POOL*)arg;
    MY_TEST_THREAD_TASK task;

    while (!thread_pool->Exit) 
    {
        pthread_mutex_lock(&thread_pool->Lock);
        MY_TEST_UATOMIC_INC(&thread_pool->ThreadNum);
        while(pthread_cond_wait(&thread_pool->Cond, &thread_pool->Lock) == 0)
        {
            if (thread_pool->Exit)
            {
                pthread_mutex_unlock(&thread_pool->Lock);
                goto CommonReturn;
            }
            else if (thread_pool->QueueCount == 0)
            {
                pthread_mutex_unlock(&thread_pool->Lock);
                continue;
            }
            else
            {
                task = thread_pool->TaskQueue[thread_pool->QueueFront];
                memset(&thread_pool->TaskQueue[thread_pool->QueueFront], 0, sizeof(MY_TEST_THREAD_TASK));
                thread_pool->QueueFront = (thread_pool->QueueFront + 1) % TASK_QUEUE_SIZE;
                thread_pool->QueueCount --;
                break;
            }
        };
        pthread_mutex_unlock(&thread_pool->Lock);
        
        MY_TEST_UATOMIC_DEC(&thread_pool->ThreadNum);
        if (task.TaskFunc)
        {
            task.TaskFunc(task.TaskArg);
            if (task.HasTimeOut == TRUE)
            {
                pthread_mutex_lock(task.TaskLock);
                pthread_cond_signal(task.TaskCond);
                pthread_mutex_unlock(task.TaskLock);
            }
            else
            {
                MY_TEST_UATOMIC_INC(&sg_TPoolTaskSucceed);
            }
        }
    }

CommonReturn:
    MY_TEST_UATOMIC_DEC(&thread_pool->ThreadNum);
    LogInfo("Thread worker %lu exit!", pthread_self());
    pthread_exit(NULL);
}

int
ThreadPoolModuleInit(
    int ThreadPoolSize,
    int Timeout
    )
{
    int loop = 0;
    int ret = 0;
    
    sg_ThreadPool = (MY_TEST_THREAD_POOL*)malloc(sizeof(MY_TEST_THREAD_POOL));
    if (!sg_ThreadPool)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }
    
    pthread_mutex_init(&sg_ThreadPool->Lock, NULL);
    pthread_cond_init(&sg_ThreadPool->Cond, NULL);
    sg_ThreadPool->QueueFront = 0;
    sg_ThreadPool->QueueRear = -1;
    sg_ThreadPool->QueueCount = 0;
    sg_ThreadPool->Exit = FALSE;
    sg_ThreadPool->ThreadNum = 0;

    sg_ThreadPool->Threads = (pthread_t*)malloc(sizeof(pthread_t) * ThreadPoolSize);

    for (loop = 0; loop < ThreadPoolSize; loop ++) {
        pthread_create(&(sg_ThreadPool->Threads[loop]), NULL, _ThreadPoolFunction, (void*)sg_ThreadPool);
    }

    sg_ThreadPoolTaskTimeout = Timeout ? Timeout : MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
    sg_TPoolModuleInited = TRUE;
    
    // to make sure workers are already
    while(1)
    {
        pthread_mutex_lock(&sg_ThreadPool->Lock);
        if (sg_ThreadPool->ThreadNum >= ThreadPoolSize)
        {
            pthread_mutex_unlock(&sg_ThreadPool->Lock);
            break;
        }
        pthread_mutex_unlock(&sg_ThreadPool->Lock);
    };
    
CommonReturn:
    return ret;
}

void
ThreadPoolModuleExit(
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

        for (loop = 0; loop < sg_ThreadPool->ThreadNum; loop++) {
            pthread_join(sg_ThreadPool->Threads[loop], NULL);
        }

        pthread_mutex_destroy(&(sg_ThreadPool->Lock));
        pthread_cond_destroy(&(sg_ThreadPool->Cond));
        free(sg_ThreadPool->Threads);
        free(sg_ThreadPool);
        sg_ThreadPool = NULL;
        sg_TPoolModuleInited = FALSE;
    }
}

// async api, TaskArg is a in arg
int
AddTaskIntoThread(
    void (*TaskFunc)(void*),
    __in void* TaskArg
    )
{
    int ret = 0;
    if (!sg_TPoolModuleInited)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    pthread_mutex_lock(&sg_ThreadPool->Lock);
    {
        if (sg_ThreadPool->QueueCount >= TASK_QUEUE_SIZE) {
            ret = MY_EBUSY;
            LogErr("Task queue is full. Task not added.");
            pthread_mutex_unlock(&sg_ThreadPool->Lock);
            goto CommonReturn;
        }
        sg_ThreadPool->QueueRear = (sg_ThreadPool->QueueRear + 1) % TASK_QUEUE_SIZE;
        sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskFunc = TaskFunc;
        sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskArg = TaskArg;
        sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].HasTimeOut = FALSE;
        sg_ThreadPool->QueueCount++;
        pthread_cond_signal(&sg_ThreadPool->Cond);
        MY_TEST_UATOMIC_INC(&sg_TPoolTaskAdded);
    }
    pthread_mutex_unlock(&sg_ThreadPool->Lock);
    
CommonReturn:
    return ret;
}

// sync api, you can care about TaskArg inout
int
AddTaskIntoThreadAndWait(
    void (*TaskFunc)(void*),
    __inout void* TaskArg
    )
{
    int ret = 0;
    struct timespec ts = {0, 0};
    BOOL taskAdded = FALSE;
    pthread_mutex_t taskLock;
    pthread_cond_t taskCond;
    BOOL pthreadTaskInited = FALSE;
    
    if (!sg_TPoolModuleInited || sg_ThreadPoolTaskTimeout <= 0)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    
    pthread_mutex_init(&taskLock, NULL);
    pthread_cond_init(&taskCond, NULL);
    pthreadTaskInited = TRUE;
    
    pthread_mutex_lock(&sg_ThreadPool->Lock);
    {
        if (sg_ThreadPool->QueueCount >= TASK_QUEUE_SIZE) {
            LogErr("Task queue is full. Task not added.");
            pthread_mutex_unlock(&(sg_ThreadPool->Lock));
            goto CommonReturn;
        }
        if (clock_gettime(CLOCK_REALTIME, &ts) == MY_SUCCESS)
        {
            sg_ThreadPool->QueueRear = (sg_ThreadPool->QueueRear + 1) % TASK_QUEUE_SIZE;
            sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskFunc = TaskFunc;
            sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskArg = TaskArg;
            sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskLock = &taskLock;
            sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskCond = &taskCond;
            sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].HasTimeOut = TRUE;
            sg_ThreadPool->QueueCount++;
            taskAdded = TRUE;
            pthread_mutex_lock(sg_ThreadPool->TaskQueue[sg_ThreadPool->QueueRear].TaskLock);

            pthread_cond_signal(&sg_ThreadPool->Cond);
            MY_TEST_UATOMIC_INC(&sg_TPoolTaskAdded);
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
        ts.tv_sec += sg_ThreadPoolTaskTimeout;
        ts.tv_nsec = 0;
        ret = pthread_cond_timedwait(&taskCond, &taskLock, &ts); 
    }
    pthread_mutex_unlock(&taskLock);
    
    if (ETIMEDOUT == ret)
    {
        pthread_mutex_lock(&sg_ThreadPool->Lock);
        sg_ThreadPool->Exit = TRUE;
        pthread_cond_broadcast(&sg_ThreadPool->Cond);
        pthread_mutex_unlock(&sg_ThreadPool->Lock);
        sg_TPoolModuleInited = FALSE;
        LogErr("Error happened, tpool exit!");
    }

CommonReturn:
    if (pthreadTaskInited)
    {
        pthread_mutex_destroy(&taskLock);
        pthread_cond_destroy(&taskCond); 
    }
    if (ret)
    {
        MY_TEST_UATOMIC_INC(&sg_TPoolTaskFailed);
    }
    else
    {
        MY_TEST_UATOMIC_INC(&sg_TPoolTaskSucceed);
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
        "<%s:[TaskAdded=%d, TaskSucceed=%d, TaskFailed=%d], ThreadNum=%d>",
            ModuleNameByEnum(MY_MODULES_ENUM_TPOOL), sg_TPoolTaskAdded, sg_TPoolTaskSucceed, sg_TPoolTaskFailed,
            sg_ThreadPool->ThreadNum);
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

