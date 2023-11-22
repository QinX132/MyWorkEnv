#include "include.h"
#include "myCommonUtil.h"
#include "myModuleCommon.h"
#include "ServerMonitor.h"

#define SERVER_DEFAULT_WARNING_CPU_USAGE                            10 // 10%

static int sg_ServerWarningCpuUsage = SERVER_DEFAULT_WARNING_CPU_USAGE;

int 
ServerCpuUsageMonitor(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = MY_SUCCESS;
    static uint64_t oldTotal = 0;
    static uint64_t oldIdle = 0;
    uint64_t newTotal = 0, newIdle = 0;
    double cpuUsage = 0.0;

    if (oldIdle == 0|| oldTotal == 0)
    {
        ret = MyUtil_GetCpuTime(&oldTotal, &oldIdle);
        if (ret)
        {
            LogErr("Get cpu time failed %d:%s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
    }
    else
    {
        MyUtil_GetCpuTime(&newTotal, &newIdle);
        if (ret)
        {
            LogErr("Get cpu time failed %d:%s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        cpuUsage = newTotal > oldTotal ? 1.0 - (double)(newIdle - oldIdle) / (newTotal - oldTotal) : 0;
        cpuUsage *= 100;

        if (cpuUsage > sg_ServerWarningCpuUsage)
        {
            *Offset += snprintf(Buff + *Offset, BuffMaxLen - *Offset, "Warning: Too big cpuusage %lf%%", cpuUsage);
        }

        oldIdle = newIdle;
        oldTotal = newTotal;
    }
    
CommonReturn:
    return ret;
}

int
ServerMonitorCmdSetCpuWarn(
    __in char* Cmd,
    __in size_t CmdLen,
    __out char* ReplyBuff,
    __out size_t ReplyBuffLen
    )
{
    int ret = MY_SUCCESS;
    int offset = 0;
    int cpuWarnValue = 0;
    char *ptr = NULL;
    UNUSED(CmdLen);

    ptr = strchr(Cmd, ' ');
    if (!ptr)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }
    cpuWarnValue = atoi(ptr + 1);
    if (cpuWarnValue < 0 || cpuWarnValue >= 100)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }
    sg_ServerWarningCpuUsage = cpuWarnValue;
    LogDbg("Set cpu warn value as %d", sg_ServerWarningCpuUsage);

CommonReturn:
    if (ret == MY_SUCCESS)
    {
        offset += snprintf(ReplyBuff + offset, ReplyBuffLen - offset, 
            "Set cpu warn value as %d", sg_ServerWarningCpuUsage);
    }
    else
    {
        offset += snprintf(ReplyBuff + offset, ReplyBuffLen - offset, 
            "Invalid cmd %s", Cmd);
    }
    return ret;
}

