#include "myThreadPool.h"
#include "myLogIO.h"

#define MY_TEST_TASK_TIME_OUT_DEFAULT_VAL       5 //seconds
static int sg_ThreadPoolTaskTimeout = MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
static BOOL sg_TPoolModuleInited = FALSE;

static void* 
_ThreadPoolFunction(
    void* arg
    )
{
    MY_TEST_THREAD_POOL* thread_pool = (MY_TEST_THREAD_POOL*)arg;

    while (1) {
        pthread_mutex_lock(&(thread_pool->Lock));

        while (thread_pool->QueueCount == 0 && !thread_pool->Exit) {
            pthread_cond_wait(&(thread_pool->Cond), &(thread_pool->Lock));
        }

        if (thread_pool->Exit) {
            pthread_mutex_unlock(&(thread_pool->Lock));
            pthread_exit(NULL);
        }

        MY_TEST_THREAD_TASK task = thread_pool->TaskQueue[thread_pool->QueueFront];
        thread_pool->QueueFront = (thread_pool->QueueFront + 1) % TASK_QUEUE_SIZE;
        memset(&thread_pool->TaskQueue[thread_pool->QueueFront], 0, sizeof(MY_TEST_THREAD_TASK));
        thread_pool->QueueCount --;

        pthread_mutex_unlock(&(thread_pool->Lock));

        if (task.TaskFunc)
        {
            task.TaskFunc(task.TaskArg);
            task.TaskDone = TRUE;
        }
    }

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
    
    pthread_mutex_init(&ThreadPool->Lock, NULL);
    pthread_cond_init(&ThreadPool->Cond, NULL);
    ThreadPool->QueueFront = 0;
    ThreadPool->QueueRear = -1;
    ThreadPool->QueueCount = 0;
    ThreadPool->Exit = FALSE;
    ThreadPool->ThreadNum = ThreadPoolSize;

    ThreadPool->Threads = (pthread_t*)malloc(sizeof(pthread_t) * ThreadPoolSize);

    for (loop = 0; loop < ThreadPoolSize; loop++) {
        pthread_create(&(ThreadPool->Threads[loop]), NULL, _ThreadPoolFunction, (void*)ThreadPool);
    }

    sg_ThreadPoolTaskTimeout = Timeout ? Timeout : MY_TEST_TASK_TIME_OUT_DEFAULT_VAL;
    sg_TPoolModuleInited = TRUE;
    LogInfo("Thread pool inited!");
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

int
AddTaskIntoThread(
    MY_TEST_THREAD_POOL* ThreadPool,
    void (*TaskFunc)(void*),
    void* TaskArg
    )
{
    int ret = 0;
    if (!sg_TPoolModuleInited)
    {
        ret = EINVAL;
        goto CommonReturn;
    }
    pthread_mutex_lock(&(ThreadPool->Lock));

    if (ThreadPool->QueueCount == TASK_QUEUE_SIZE) {
        LogErr("Task queue is full. Task not added.\n");
        pthread_mutex_unlock(&(ThreadPool->Lock));
        goto CommonReturn;
    }

    ThreadPool->QueueRear = (ThreadPool->QueueRear + 1) % TASK_QUEUE_SIZE;
    ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskFunc = TaskFunc;
    ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskArg = TaskArg;
    ThreadPool->TaskQueue[ThreadPool->QueueRear].TaskDone = FALSE;
    ThreadPool->QueueCount++;

    pthread_cond_signal(&(ThreadPool->Cond));
    pthread_mutex_unlock(&(ThreadPool->Lock));
    
CommonReturn:
    return ret;
}
