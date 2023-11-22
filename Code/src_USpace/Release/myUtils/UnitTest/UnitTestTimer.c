#include "UnitTest.h"
#include "myTimer.h"
#include "myCommonUtil.h"
#include "myMem.h"

#define UT_TIMER_CB_TIMEVAL                                 500 //ms
#define UT_TIMER_WAIT_TIME                                  3 //s
#define UT_TIMER_ONT_TIME_ARG                               "UT_TIMER_ONT_TIME"
#define UT_TIMER_LOOP_ARG                                   "UT_TIMER_LOOP"

static int sg_UT_LoopTimerCbCalled = 0;
static BOOL sg_UT_OneTimeTimerCbCalled = FALSE;
static BOOL sg_UT_TimerErrHappend = FALSE;

void
_UT_Timer_OneTimeFunc(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    double cpuUsage = 0;
    char* arg = (char*)Arg;
    UNUSED(Fd);
    UNUSED(Event);
    
    sg_UT_OneTimeTimerCbCalled = TRUE;
    
MY_UTIL_GET_CPU_USAGE_START
{
    usleep(10);
}
MY_UTIL_GET_CPU_USAGE_END(cpuUsage);
    
    UTLog("<%s:cpuUsage=%lf>\n", arg, cpuUsage);

    if (strcmp(arg, UT_TIMER_ONT_TIME_ARG) != 0)
    {
        sg_UT_TimerErrHappend = TRUE;
    }
}

void
_UT_Timer_LoopFunc(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    double cpuUsage = 0;
    char* arg = (char*)Arg;
    UNUSED(Fd);
    UNUSED(Event);
    
    sg_UT_LoopTimerCbCalled ++;
    
MY_UTIL_GET_CPU_USAGE_START
{
    usleep(10);
}
MY_UTIL_GET_CPU_USAGE_END(cpuUsage);
    
    UTLog("<%s:cpuUsage=%lf>\n", arg, cpuUsage);
    
    if (strcmp(arg, UT_TIMER_LOOP_ARG) != 0)
    {
        sg_UT_TimerErrHappend = TRUE;
    }
}
    
static int
_UT_Timer_PreInit(
    void
    )
{
    return MemModuleInit();
}

static int
_UT_Timer_FinExit(
    void
    )
{
    return MemModuleExit();
}

static int
_UT_Timer_ForwardT(
    void
    )
{
    int ret = MY_SUCCESS;
    TIMER_HANDLE handle = NULL;
    
    ret = TimerModuleInit();
    if (ret)
    {
        UTLog("Health init failed!\n");
        goto CommonReturn;
    }

    ret = TimerAdd(_UT_Timer_OneTimeFunc, UT_TIMER_CB_TIMEVAL, (void*)UT_TIMER_ONT_TIME_ARG, 
                    MY_TIMER_TYPE_ONE_TIME, NULL);
    if (ret)
    {
        UTLog("Timer one time add failed!\n");
        goto CommonReturn;
    }

    ret = TimerAdd(_UT_Timer_LoopFunc, UT_TIMER_CB_TIMEVAL, (void*)UT_TIMER_LOOP_ARG, 
                    MY_TIMER_TYPE_LOOP, &handle);
    if (ret || !handle)
    {
        UTLog("Timer loop add failed!\n");
        goto CommonReturn;
    }
    
    sleep(UT_TIMER_WAIT_TIME);
    if (sg_UT_LoopTimerCbCalled < UT_TIMER_WAIT_TIME*1000 / UT_TIMER_CB_TIMEVAL - 2)
    {
        ret = -MY_EIO;
        UTLog("Timer loop func called %d.\n", sg_UT_LoopTimerCbCalled);
        goto CommonReturn;
    }
    if (!sg_UT_OneTimeTimerCbCalled)
    {
        ret = -MY_EIO;
        UTLog("Timer one time ont called.\n");
        goto CommonReturn;
    }

    TimerDel(&handle);
    if (handle)
    {
        UTLog("Timer loop del failed!\n");
        goto CommonReturn;
    }
    
CommonReturn:
    if (TimerModuleExit())
    {
        ret = -MY_EINVAL;
    }
    if (!MemLeakSafetyCheck())
    {
        ret = -MY_EINVAL;
    }
    return ret;
}


TEST(UnitTest, Timer)
{
    ASSERT_EQ(MY_SUCCESS, _UT_Timer_PreInit());
    EXPECT_EQ(MY_SUCCESS, _UT_Timer_ForwardT());
    ASSERT_EQ(MY_SUCCESS, _UT_Timer_FinExit());
}

TEST(UnitTest, TimerErrCheck)
{
    ASSERT_EQ(FALSE, sg_UT_TimerErrHappend);
}

