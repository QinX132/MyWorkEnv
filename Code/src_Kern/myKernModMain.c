#include "include.h"

static int 
_CBFunc(
    void* arg
    )
{
    while(!kthread_should_stop())
    {
        ssleep(1);
        LogInfo("thread : %d\n", *((int*)arg));
        (*((int*)arg)) ++;
    }
    
    return 0;
}

static int __init _myModuleInit(void)
{
    typedef struct task_struct* LW_THREAD_T;
    int arg = 0;

    LW_THREAD_T tid;
    tid = kthread_run(_CBFunc, (void*)&arg, "myTestT");
    
    ssleep(5);
    LogInfo("main : %d\n", arg);
    kthread_stop(tid);
    ssleep(5);

    return 0;
}

static void __exit _myModuleExit(void)
{
    return ;
}

module_init(_myModuleInit);
module_exit(_myModuleExit);

