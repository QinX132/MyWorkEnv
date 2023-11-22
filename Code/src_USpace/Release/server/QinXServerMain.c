#include "include.h"
#include "myModuleCommon.h"
#include "ServerProc.h"
#include "ServerMonitor.h"

#define MY_SERVER_ROLE_NAME                                 "QinXServer"
#define MY_SERVER_CONF_ROOT                                 MY_SERVER_ROLE_NAME".conf"

typedef struct _SERVER_CONF_PARAM
{
    MY_LOG_LEVEL LogLevel;
    char LogFilePath[MY_BUFF_64];
    int LogMaxSize;
    int LogMaxNum;
    MY_HEALTH_MODULE_INIT_ARG HealthArg;
    MY_TPOOL_MODULE_INIT_ARG TPoolArg;
}
SERVER_CONF_PARAM;

static int 
_ServerParseConf(
    SERVER_CONF_PARAM *ServerConf
    )
{
    int ret = MY_SUCCESS;
    FILE *fp = NULL;
    char line[MY_BUFF_128] = {0};
    char *ptr = NULL;
    int len = 0;

    fp = fopen(MY_SERVER_CONF_ROOT, "r");
    if (!fp)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }
    while(fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#')
        {
            continue;
        }
        else if ((ptr = strstr(line, "LogLevel=")) != NULL)
        {
            ServerConf->LogLevel = atoi(ptr + strlen("LogLevel="));
            if (!(ServerConf->LogLevel >= MY_LOG_LEVEL_INFO && ServerConf->LogLevel <= MY_LOG_LEVEL_ERROR))
            {
                ret = -MY_EIO;
                LogErr("Invalid loglevel!");
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "LogPath=")) != NULL)
        {
            len = snprintf(ServerConf->LogFilePath, sizeof(ServerConf->LogFilePath), "%s", ptr + strlen("LogPath="));
            if (!len)
            {
                ret = -MY_EIO;
                LogErr("Invalid logPath %s!", ptr + strlen("LogPath="));
                goto CommonReturn;
            }
            if ('\n' == ServerConf->LogFilePath[len - 1])
            {
                ServerConf->LogFilePath[len - 1] = '\0';
            }
        }
        else if ((ptr = strstr(line, "LogMaxSize=")) != NULL)
        {
            ServerConf->LogMaxSize = atoi(ptr + strlen("LogMaxSize=")) * 1024 * 1024;
            if (ServerConf->LogMaxSize <= 0)
            {
                ret = -MY_EIO;
                LogErr("Invalid LogMaxSize!");
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "LogMaxNum=")) != NULL)
        {
            ServerConf->LogMaxNum = atoi(ptr + strlen("LogMaxNum="));
            if (ServerConf->LogMaxNum <= 0)
            {
                ret = -MY_EIO;
                LogErr("Invalid LogMaxNum!");
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "LogHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.LogHealthIntervalS = atoi(ptr + strlen("LogHealthIntervalS="));
        }
        // module health check
        else if ((ptr = strstr(line, "MsgHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.MsgHealthIntervalS = atoi(ptr + strlen("MsgHealthIntervalS="));
        }
        else if ((ptr = strstr(line, "TPoolHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.TPoolHealthIntervalS = atoi(ptr + strlen("TPoolHealthIntervalS="));
        }
        else if ((ptr = strstr(line, "CmdLineHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.CmdLineHealthIntervalS = atoi(ptr + strlen("CmdLineHealthIntervalS="));
        }
        else if ((ptr = strstr(line, "MHHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.MHHealthIntervalS = atoi(ptr + strlen("MHHealthIntervalS="));
        }
        else if ((ptr = strstr(line, "MemHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.MemHealthIntervalS = atoi(ptr + strlen("MemHealthIntervalS="));
        }
        else if ((ptr = strstr(line, "TimerHealthIntervalS=")) != NULL)
        {
            ServerConf->HealthArg.TimerHealthIntervalS = atoi(ptr + strlen("TimerHealthIntervalS="));
        }
        // TPool param
        else if ((ptr = strstr(line, "TPoolSize=")) != NULL)
        {
            ServerConf->TPoolArg.ThreadPoolSize = atoi(ptr + strlen("TPoolSize="));
            if (ServerConf->TPoolArg.ThreadPoolSize < 0)
            {
                ret = -MY_EIO;
                LogErr("Invalid TPoolSize %d!", ServerConf->TPoolArg.ThreadPoolSize);
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "TPoolDefaultTimeout=")) != NULL)
        {
            ServerConf->TPoolArg.Timeout = atoi(ptr + strlen("TPoolDefaultTimeout="));
            if (ServerConf->TPoolArg.Timeout < 0)
            {
                ret = -MY_EIO;
                LogErr("Invalid TPoolDefaultTimeout %d!", ServerConf->TPoolArg.Timeout);
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "TPoolTaskQueueMaxLength=")) != NULL)
        {
            ServerConf->TPoolArg.TaskListMaxLength = atoi(ptr + strlen("TPoolTaskQueueMaxLength="));
            if (ServerConf->TPoolArg.TaskListMaxLength < 0)
            {
                ret = -MY_EIO;
                LogErr("Invalid TaskListMaxLength %d!", ServerConf->TPoolArg.TaskListMaxLength);
                goto CommonReturn;
            }
        }
        memset(line, 0, sizeof(line));
    }

CommonReturn:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}

static void
_ServerExit(
    void
    )
{
    // msg handler
    ServerProcExit();
    LogInfo("----------------- ServerProc exited! -------------------");

    MyModuleCommonExit();
    system("killall "MY_SERVER_ROLE_NAME);
}

static int
_ServerCmdRegister(
    void
    )
{
    int ret = MY_SUCCESS;
    MY_CMD_EXTERNAL_CONT cont;

// add proc stats
    snprintf(cont.Opt, sizeof(cont.Opt), "ShowServerProcStat");
    snprintf(cont.Help, sizeof(cont.Help), "show current connected clients info");
    cont.Argc = 2;
    cont.Cb = ServerProcCmdShowStats;
    ret = CmdExternalRegister(cont);
    if (ret)
    {
        LogErr("Cmd Register failed!");
        goto CommonReturn;
    }
// monitor : cpuusgae warning value
    snprintf(cont.Opt, sizeof(cont.Opt), "SetServerCpuUsageWarningValue");
    snprintf(cont.Help, sizeof(cont.Help), "<PERSENT %%> Set server cpuusage warning value");
    cont.Argc = 3;
    cont.Cb = ServerMonitorCmdSetCpuWarn;
    ret = CmdExternalRegister(cont);
    if (ret)
    {
        LogErr("Cmd Register failed!");
        goto CommonReturn;
    }
CommonReturn:
    return ret;
}

static int
_ServerHealthRegister(
    void
    )
{
    int ret = MY_SUCCESS;

    ret = HealthMonitorAdd(ServerCpuUsageMonitor, "CpuUsageMonitor", SERVER_DEFAULT_CPU_USAGE_MONITOR_INTERVAL_S);
    if (ret)
    {
        LogErr("Cmd Register failed!");
        goto CommonReturn;
    }

CommonReturn:
    return ret;
}

static int
_ServerInit(
    int argc,
    char *argv[]
    )
{
    int ret = MY_SUCCESS;
    MY_MODULES_INIT_PARAM initParam;
    SERVER_CONF_PARAM serverConf;
    
    memset(&initParam, 0, sizeof(initParam));
    memset(&serverConf, 0, sizeof(serverConf));

    ret = _ServerCmdRegister();
    if (ret)
    {
        LogErr("Cmd register failed!");
        goto CommonReturn;
    }

    ret = _ServerParseConf(&serverConf);
    if (ret)
    {
        LogErr("Init conf failed!");
        goto CommonReturn;
    }

    initParam.InitMsgModule = TRUE;
    initParam.InitTimerModule = TRUE;
    initParam.CmdLineArg = (MY_CMDLINE_MODULE_INIT_ARG*)calloc(sizeof(MY_CMDLINE_MODULE_INIT_ARG), 1);
    initParam.LogArg = (MY_LOG_MODULE_INIT_ARG*)calloc(sizeof(MY_LOG_MODULE_INIT_ARG), 1);
    if (!initParam.CmdLineArg || !initParam.LogArg)
    {
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    // health init args
    initParam.HealthArg = &serverConf.HealthArg;
    // cmd line init args
    initParam.CmdLineArg->Argc = argc;
    initParam.CmdLineArg->Argv = argv;
    initParam.CmdLineArg->RoleName = MY_SERVER_ROLE_NAME;
    initParam.CmdLineArg->ExitFunc = _ServerExit;
    // log init args
    initParam.LogArg->LogFilePath = serverConf.LogFilePath;
    initParam.LogArg->LogLevel = serverConf.LogLevel;
    initParam.LogArg->RoleName = MY_SERVER_ROLE_NAME;
    initParam.LogArg->LogMaxSize = serverConf.LogMaxSize;
    initParam.LogArg->LogMaxNum = serverConf.LogMaxNum;
    // tpool init args
    initParam.TPoolArg = &serverConf.TPoolArg;
    ret = MyModuleCommonInit(initParam);
    if (ret)
    {
        if (-MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("MyModuleCommonInit failed!");
        }
        goto CommonReturn;
    }
    
    ret = ServerProcInit();
    if (ret)
    {
        LogErr("ServerProc init Failed!");
        goto CommonReturn;
    }
    ret = _ServerHealthRegister();
    if (ret)
    {
        LogErr("Server Health register Failed!");
        goto CommonReturn;
    }
CommonReturn:
    if (initParam.LogArg)
    {
        free(initParam.LogArg);
    }
    if (initParam.CmdLineArg)
    {
        free(initParam.CmdLineArg);
    }
    if (ret && -MY_ERR_EXIT_WITH_SUCCESS != ret)
    {
        ServerProcExit();
        MyModuleCommonExit();
    }
    return ret;
}

int
main(
    int argc,
    char *argv[]
    )
{
    int ret = MY_SUCCESS;
    ret = _ServerInit(argc, argv);
    if (ret)
    {
        if (-MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("Server init failed! ret %d", ret);
        }
        goto CommonReturn;
    }
    
    while(1)
    {
        sleep(60);
    }
    
CommonReturn:
    return ret;
}
