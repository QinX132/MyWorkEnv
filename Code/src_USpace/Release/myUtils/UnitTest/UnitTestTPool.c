#include "UnitTest.h"
#include "myThreadPool.h"
#include "myCommonUtil.h"
#include "myMem.h"
#include "myLogIO.h"

#define UT_TPOOL_TEST_VAL                                           132
#define UT_TPOOL_TEST_TIMEOUT_VAL                                   133
#define UT_TPOOL_MAX_CPU_USAGE                                      0.1     // 10%

typedef struct _UT_TPOOL_TASK_ARG
{
    int Value;
    void* Ptr;
}
UT_TPOOL_TASK_ARG;

static BOOL sg_UT_TPool_TaskErrHapped = FALSE;

static int
_UT_TPool_PreInit(
    void
    )
{
    return MemModuleInit();
}

static int
_UT_TPool_FinExit(
    void
    )
{
    return MemModuleExit();
}

static void
_UT_TPool_AddTaskCb(
    void* Arg
    )
{
    UT_TPOOL_TASK_ARG *taskArg = (UT_TPOOL_TASK_ARG *)Arg;

    if ((taskArg->Value != UT_TPOOL_TEST_VAL && taskArg->Value != UT_TPOOL_TEST_TIMEOUT_VAL) || taskArg->Ptr != Arg)
    {
        sg_UT_TPool_TaskErrHapped = TRUE;
        UTLog("Invalid arg, value:%d arg:%p taskarg:%p\n", taskArg->Value, taskArg, taskArg->Ptr);
    }
    if (taskArg->Value == UT_TPOOL_TEST_TIMEOUT_VAL)
    {
        UTLog("Waiting ...\n");
        usleep(1500 * 1000);
    }
    MyFree(taskArg);
    UTLog("_TPool_AddTaskCb success\n");
}

static int
_UT_TPool_InitExit(
    void
    )
{
    int ret = MY_SUCCESS;
    MY_TPOOL_MODULE_INIT_ARG initArg = {.ThreadPoolSize = 256, .Timeout = 5, .TaskListMaxLength = 1024};

    LogSetLevel(1);
    
    ret = TPoolModuleInit(&initArg);
    if (ret)
    {
        UTLog("Init fail\n");
        goto CommonReturn;
    }

CommonReturn:
    if (TPoolModuleExit())
    {
        ret = -MY_EINVAL;
    }
    if (!MemLeakSafetyCheck())
    {
        ret = -MY_EINVAL;
    }
    LogSetLevel(0);
    return ret;
}

static int
_UT_TPool_ForwardT(
    void
    )
{
    int ret = MY_SUCCESS;
    UT_TPOOL_TASK_ARG *taskArg = NULL;
    double cpuUsage = 0;
    MY_TPOOL_MODULE_INIT_ARG initArg = {.ThreadPoolSize = 3, .Timeout = 5, .TaskListMaxLength = 1024};
    
MY_UTIL_GET_CPU_USAGE_START
{
    ret = TPoolModuleInit(&initArg);
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
    ret = TPoolAddTask(_UT_TPool_AddTaskCb, (void*)taskArg);
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
    ret = TPoolAddTaskAndWait(_UT_TPool_AddTaskCb, (void*)taskArg, 5);
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
    ret = TPoolAddTask(_UT_TPool_AddTaskCb, (void*)taskArg);
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
    taskArg->Value = UT_TPOOL_TEST_TIMEOUT_VAL;
    taskArg->Ptr = (void*)taskArg;
    ret = TPoolAddTaskAndWait(_UT_TPool_AddTaskCb, (void*)taskArg, 1);
    if (ret != -MY_ETIMEDOUT)
    {
        UTLog("wait fail, ret %d\n", ret);
        ret = -MY_EIO;
        goto CommonReturn;
    }
    else
    {
        ret = MY_SUCCESS;
    }
    sleep(1);
    usleep(100 * 100);
}
MY_UTIL_GET_CPU_USAGE_END(cpuUsage);
    UTLog("Cpu usage %lf\n", cpuUsage);

    if (cpuUsage > UT_TPOOL_MAX_CPU_USAGE)
    {
        ret = -MY_E2BIG;
    }
    
CommonReturn:
    if (TPoolModuleExit())
    {
        ret = -MY_EINVAL;
    }
    if (!MemLeakSafetyCheck())
    {
        ret = -MY_EINVAL;
    }
    return ret;
}


TEST(UnitTest, TPoolInit)
{
    ASSERT_EQ(MY_SUCCESS, _UT_TPool_PreInit());
    EXPECT_EQ(MY_SUCCESS, _UT_TPool_InitExit());
    ASSERT_EQ(MY_SUCCESS, _UT_TPool_FinExit());
}

TEST(UnitTest, TPool)
{
    ASSERT_EQ(MY_SUCCESS, _UT_TPool_PreInit());
    EXPECT_EQ(MY_SUCCESS, _UT_TPool_ForwardT());
    ASSERT_EQ(MY_SUCCESS, _UT_TPool_FinExit());
}

TEST(UnitTest, TPoolErrCheck)
{
    EXPECT_EQ(FALSE, sg_UT_TPool_TaskErrHapped);
}
