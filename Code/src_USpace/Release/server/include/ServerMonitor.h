#ifndef _SERVER_MONITOR_H_
#define _SERVER_MONITOR_H_

#include "include.h"
    
#ifdef __cplusplus
    extern "C"{
#endif

#define SERVER_DEFAULT_CPU_USAGE_MONITOR_INTERVAL_S             10 //s

int 
ServerCpuUsageMonitor(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );
    
int
ServerMonitorCmdSetCpuWarn(
    __in char* Cmd,
    __in size_t CmdLen,
    __out char* ReplyBuff,
    __out size_t ReplyBuffLen
    );

#ifdef __cplusplus
}
#endif

#endif /* _SERVER_MONITOR_H_ */

