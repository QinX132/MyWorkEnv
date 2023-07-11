#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myLogIO.h"

#define MY_TEST_TASK_TIME_OUT_DEFAULT_VAL                       5 //seconds
static int sg_ThreadPoolTaskTimeout = MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
static BOOL sg_TPoolModuleInited = FALSE;

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

void
ThreadPoolModuleInit(
    MY_TEST_THREAD_POOL* ThreadPool,
    int ThreadPoolSize,
    int Timeout
    )
{
    int loop = 0;
    LogInfo("Thread pool init start!");
    
    pthread_mutex_init(&ThreadPool->Lock, NULL);
    pthread_cond_init(&ThreadPool->Cond, NULL);
    ThreadPool->QueueFront = 0;
    ThreadPool->QueueRear = -1;
    ThreadPool->QueueCount = 0;
    ThreadPool->Exit = FALSE;
    ThreadPool->ThreadNum = 0;

    ThreadPool->Threads = (pthread_t*)malloc(sizeof(pthread_t) * ThreadPoolSize);

    for (loop = 0; loop < ThreadPoolSize; loop ++) {
        pthread_create(&(ThreadPool->Threads[loop]), NULL, _ThreadPoolFunction, (void*)ThreadPool);
    }

    sg_ThreadPoolTaskTimeout = Timeout ? Timeout : MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
    sg_TPoolModuleInited = TRUE;
    
    // to make sure workers are already
    while(1)
    {
        pthread_mutex_lock(&ThreadPool->Lock);
        if (ThreadPool->ThreadNum >= ThreadPoolSize)
        {
            pthread_mutex_unlock(&ThreadPool->Lock);
            break;
        }
        pthread_mutex_unlock(&ThreadPool->Lock);
    };
    
    LogInfo("Thread pool init success!");
}

void
ThreadPoolModuleExit(
    MY_TEST_THREAD_POOL* ThreadPool
    )
{
    int loop = 0;

    if (sg_TPoolModuleInited)
    {
        ThreadPool->Exit = TRUE;
        pthread_mutex_lock(&(ThreadPool->Lock));
        pthread_cond_broadcast(&(ThreadPool->Cond));
        pthread_mutex_unlock(&(ThreadPool->Lock));

        for (loop = 0; loop < ThreadPool->ThreadNum; loop++) {
            pthread_join(ThreadPool->Threads[loop], NULL);
        }

        pthread_mutex_destroy(&(ThreadPool->Lock));
        pthread_cond_destroy(&(ThreadPool->Cond));
        free(ThreadPool->Threads);
        LogInfo("Thread pool exited!");
    }
}

// async api, TaskArg is a in arg
int
AddTaskIntoThread(
    MY_TEST_THREAD_POOL* ThreadPool,
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
    pthread_mutex_lock(&ThreadPool->Lock);
    {
        if (ThreadPool->QueueCount >= TASK_QUEUE_SIZE) {
            ret = MY_EBUSY;
            LogErr("Task queue is full. Task not added.");
            pthread_mutex_unlock(&ThreadPool->Lock);
            goto CommonReturn;
        }
        ThreadPool->QueueRear = (ThreadPool->QueueRear + 1) % TASK_QUEUE_SIZE;
        ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskFunc = TaskFunc;
        ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskArg = TaskArg;
        ThreadPool->TaskQueue[ThreadPool->QueueRear].HasTimeOut = FALSE;
        ThreadPool->QueueCount++;
        pthread_cond_signal(&ThreadPool->Cond);
        MY_TEST_UATOMIC_INC(&sg_TPoolTaskAdded);
    }
    pthread_mutex_unlock(&ThreadPool->Lock);
    
CommonReturn:
    MY_TEST_UATOMIC_INC(&sg_TPoolTaskSucceed);
    return ret;
}

// sync api, you can care about TaskArg inout
int
AddTaskIntoThreadAndWait(
    MY_TEST_THREAD_POOL* ThreadPool,
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
    
    pthread_mutex_lock(&ThreadPool->Lock);
    {
        if (ThreadPool->QueueCount >= TASK_QUEUE_SIZE) {
            LogErr("Task queue is full. Task not added.");
            pthread_mutex_unlock(&(ThreadPool->Lock));
            goto CommonReturn;
        }
        if (clock_gettime(CLOCK_REALTIME, &ts) == MY_SUCCESS)
        {
            ThreadPool->QueueRear = (ThreadPool->QueueRear + 1) % TASK_QUEUE_SIZE;
            ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskFunc = TaskFunc;
            ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskArg = TaskArg;
            ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskLock = &taskLock;
            ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskCond = &taskCond;
            ThreadPool->TaskQueue[ThreadPool->QueueRear].HasTimeOut = TRUE;
            ThreadPool->QueueCount++;
            taskAdded = TRUE;
            pthread_mutex_lock(ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskLock);

            pthread_cond_signal(&ThreadPool->Cond);
            MY_TEST_UATOMIC_INC(&sg_TPoolTaskAdded);
        }
        else
        {
            ret = MY_EIO;
            LogErr("Get time failed!");
        }
    }
    pthread_mutex_unlock(&ThreadPool->Lock);

    if (taskAdded)
    {
        ts.tv_sec += sg_ThreadPoolTaskTimeout;
        ts.tv_nsec = 0;
        ret = pthread_cond_timedwait(&taskCond, &taskLock, &ts); 
    }
    pthread_mutex_unlock(&taskLock);
    
    if (ETIMEDOUT == ret)
    {
        pthread_mutex_lock(&ThreadPool->Lock);
        ThreadPool->Exit = TRUE;
        pthread_cond_broadcast(&ThreadPool->Cond);
        pthread_mutex_unlock(&ThreadPool->Lock);
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

void
TPoolModuleStat(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    UNUSED(Fd);
    UNUSED(Event);
    UNUSED(Arg);

    LogInfo("<%s:[TaskAdded=%d, TaskSucceed=%d, TaskFailed=%d]>",
        HealthModuleNameByEnum(MY_MODULES_ENUM_TPOOL), sg_TPoolTaskAdded, sg_TPoolTaskSucceed, sg_TPoolTaskFailed);
}
