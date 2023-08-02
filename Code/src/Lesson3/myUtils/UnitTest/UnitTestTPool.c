#include "UnitTest.h"
#include "myThreadPool.h"
#include "myCommonUtil.h"
#include "myMem.h"

#define UT_TPOOL_TEST_VAL                                           132
#define UT_TPOOL_MAX_CPU_USAGE                                      0.5     // 50%

typedef struct _UT_TPOOL_TASK_ARG
{
    int Value;
    void* Ptr;
}
UT_TPOOL_TASK_ARG;

static BOOL sg_TPoolTaskErrHapped = FALSE;

static void
_TPool_AddTaskCb(
    void* Arg
    )
{
    UT_TPOOL_TASK_ARG *taskArg = (UT_TPOOL_TASK_ARG *)Arg;

    if (taskArg->Value != UT_TPOOL_TEST_VAL || taskArg->Ptr != Arg)
    {
        sg_TPoolTaskErrHapped = TRUE;
        UTLog("Invalid arg, value:%d arg:%p taskarg:%p\n", taskArg->Value, taskArg, taskArg->Ptr);
    }

    MyFree(taskArg);
}

int
_UnitTest_TPoolForwardT(
    void
    )
{
    int ret = 0;
    UT_TPOOL_TASK_ARG *taskArg = NULL;
    double cpuUsage = 0;
    MY_TPOOL_MODULE_INIT_ARG initArg = {.ThreadPoolSize = 5, .Timeout = 5};

    ret = MemModuleInit();
    if (ret)
    {
        UTLog("Init mem fail\n");
        goto CommonReturn;
    }
    
MY_UTIL_GET_CPU_USAGE_START
{
    ret = ThreadPoolModuleInit(&initArg);
    if (ret)
    {
        UTLog("Init fail\n");
        goto CommonReturn;
    }

    taskArg = (UT_TPOOL_TASK_ARG*)MyCalloc(sizeof(UT_TPOOL_TASK_ARG));
    if (!taskArg)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    taskArg->Value = UT_TPOOL_TEST_VAL;
    taskArg->Ptr = (void*)taskArg;
    ret = AddTaskIntoThread(_TPool_AddTaskCb, (void*)taskArg);
    if (ret)
    {
        UTLog("Add fail\n");
        goto CommonReturn;
    }

    taskArg = (UT_TPOOL_TASK_ARG*)MyCalloc(sizeof(UT_TPOOL_TASK_ARG));
    if (!taskArg)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    taskArg->Value = UT_TPOOL_TEST_VAL;
    taskArg->Ptr = (void*)taskArg;
    ret = AddTaskIntoThreadAndWait(_TPool_AddTaskCb, (void*)taskArg);
    if (ret)
    {
        UTLog("Add fail\n");
        goto CommonReturn;
    }
}
MY_UTIL_GET_CPU_USAGE_END(cpuUsage);
    UTLog("Cpu usage %lf\n", cpuUsage);

    if (cpuUsage > UT_TPOOL_MAX_CPU_USAGE)
    {
        ret = MY_E2BIG;
        goto CommonReturn;
    }

CommonReturn:
    ThreadPoolModuleExit();
    if (!MemLeakSafetyCheck())
    {
        ret = MY_EINVAL;
    }
    MemModuleExit();
    return ret;
}


TEST(UnitTest, TPool)
{
    EXPECT_EQ(0, _UnitTest_TPoolForwardT());
}

TEST(UnitTest, TPoolErrCheck)
{
    EXPECT_EQ(FALSE, sg_TPoolTaskErrHapped);
}
