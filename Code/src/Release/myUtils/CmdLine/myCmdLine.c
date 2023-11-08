#define _GNU_SOURCE

#include "myLogIO.h"
#include "myCmdLine.h"
#include "myCommonUtil.h"
#include "myModuleHealth.h"

typedef struct _MY_CMDLINE_CONT
{
    char* Opt;
    char* Help;
}
MY_CMDLINE_CONT;

typedef struct _MY_CMNLINE_WORKER_STATS
{
    int ExecCnt;
    int ConnectCnt;
}
MY_CMNLINE_WORKER_STATS;

typedef enum _MY_CMDLINE_ROLE
{
    MY_CMDLINE_ROLE_SVR,
    MY_CMDLINE_ROLE_CLT,
    
    MY_CMDLINE_ROLE_UNUSED
}
MY_CMDLINE_ROLE;


typedef struct _MY_CMNLINE_WORKER
{
    MY_CMDLINE_ROLE Role;
    pthread_t *Thread;
    ExitHandle ExitCb;
    BOOL Exit;
    MY_CMNLINE_WORKER_STATS Stat;
}
MY_CMNLINE_WORKER;


static MY_CMNLINE_WORKER sg_CmdLineWorker = {
        .Role = MY_CMDLINE_ROLE_UNUSED, 
        .Thread = NULL,
        .ExitCb = NULL,
        .Exit = TRUE,
        .Stat = { .ExecCnt = 0, .ConnectCnt = 0}
    };

#define MY_CMDLINE_ARG_LIST                 \
        __MY_CMDLINE_ARG("start", "start this program")  \
        __MY_CMDLINE_ARG("stop", "stop this program")  \
        __MY_CMDLINE_ARG("help", "show this page")  \
        __MY_CMDLINE_ARG("showModuleStat", "show this program's modules stats")  \
        __MY_CMDLINE_ARG("changeTPoolTimeout", "<Timout> (second)")  \
        __MY_CMDLINE_ARG("changeTPoolMaxQueueLength", "<legnth> set tpool max que length")  \
        __MY_CMDLINE_ARG("changeLogLevel", "<Level> (0-info 1-debug 2-warn 3-error)")

typedef enum _MY_CMD_TYPE
{
    MY_CMD_TYPE_START,
    MY_CMD_TYPE_STOP,
    MY_CMD_TYPE_HELP,
    MY_CMD_TYPE_SHOWSTAT,
    MY_CMD_TYPE_CHANGE_TPOOL_TIMEOUT,
    MY_CMD_TYPE_CHANGE_TPOOL_QUEUE_LENGTH,
    MY_CMD_TYPE_CHANGE_LOG_LEVEL,
    
    MY_CMD_TYPE_MAX_UNUSED
}
MY_CMD_TYPE;

static const MY_CMDLINE_CONT sg_CmdLineCont[MY_CMD_TYPE_MAX_UNUSED] = 
{
#undef __MY_CMDLINE_ARG
#define __MY_CMDLINE_ARG(_opt_,_help_) \
    {_opt_, _help_},
    MY_CMDLINE_ARG_LIST
#undef __MY_CMDLINE_ARG
};

void
_CmdLineUsage(
    char* RoleName
    )
{
    printf("--------------------------------------------------------------------------\n");
    printf("%10s Usage:\n\n", RoleName ? RoleName : "CmdLine");
#undef __MY_CMDLINE_ARG
#define __MY_CMDLINE_ARG(_opt_,_help_) \
    printf("%30s: [%-30s]\n", _opt_, _help_);
    MY_CMDLINE_ARG_LIST
#undef __MY_CMDLINE_ARG
    printf("\n--------------------------------------------------------------------------\n");
}

int 
_CmdServer_Init(
    __inout int *ServerFd,
    __inout int *EpollFd
    )
{
    int ret = 0;
    int epoll_fd = -1;
    struct epoll_event event;
    int serverFd = -1;
    int32_t reuseable = 1; // set port reuseable when fd closed
    int nonBlock = 0;
    struct sockaddr_in localAddr = {0};

    if (!ServerFd || !EpollFd)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }

    /* init server fd */
    // set reuseable
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    (void)setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));
    // set fd nonBlock
    nonBlock = fcntl(serverFd, F_GETFL, 0);
    nonBlock |= O_NONBLOCK;
    fcntl(serverFd, F_SETFL, nonBlock);
    // bind
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(MY_SERVER_CMD_LINE_PORT);
    localAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
    {
        ret = -1;
        LogErr("Bind failed");
        fflush(stdout);
        goto CommonReturn;
    }
    if(0 > listen(serverFd, 5))
    {
        ret = -1;
        LogErr("Listen failed");
        fflush(stdout);
        goto CommonReturn;
    }

    /* init epool */
    epoll_fd = epoll_create1(0);
    if (0 > epoll_fd)
    {
        ret = -1;
        LogErr("Create epoll socket failed %d", errno);
        fflush(stdout);
        goto CommonReturn;
    }
    event.events = EPOLLIN; // LT fd
    event.data.fd = serverFd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverFd, &event))
    {
        ret = -1;
        LogErr("Add to epoll socket failed %d", errno);
        fflush(stdout);
        goto CommonReturn;
    }

CommonReturn:
    if (epoll_fd <= 0 || serverFd <= 0)
    {
        ret = MY_EIO;
        LogErr("Create fd failed!");
    }
    else
    {
        *EpollFd = epoll_fd;
        *ServerFd = serverFd;
    }
    return ret;
}

int
_CmdServerHandleMsg(
    int Fd,
    char* Buff
    )
{
    int ret = 0;
    int len = 0;
    char retString[MY_BUFF_1024] = {0};

    MY_UATOMIC_INC(&sg_CmdLineWorker.Stat.ExecCnt);
    
    if (strcasestr(Buff, sg_CmdLineCont[MY_CMD_TYPE_STOP].Opt))
    {
        len = send(Fd, "Process stopped.", sizeof("Process stopped."), 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        if (sg_CmdLineWorker.ExitCb)
        {
            sg_CmdLineWorker.ExitCb();
        }
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_CMD_TYPE_SHOWSTAT].Opt))
    {
        char statBuff[MY_BUFF_1024 * MY_BUFF_1024] = {0};
        int offset = 0;
        int loop = 0;
        statBuff[offset ++] = '\n';
        for(loop = 0; loop < MY_MODULES_ENUM_MAX; loop ++)
        {
            if (sg_ModuleReprt[loop].Cb)
            {
                ret = sg_ModuleReprt[loop].Cb(statBuff, sizeof(statBuff), &offset);
                if (ret)
                {
                    LogErr("Get module report failed! ret %d!", ret);
                    break;
                }
                statBuff[offset ++] = '\n';
            }
        }
        if (ret)
        {
            memset(statBuff, 0, sizeof(statBuff));
            offset = strlen("Get module stats failed!");
            strcpy(statBuff, "Get module stats failed!");
        }
        len = send(Fd, statBuff, offset + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        else
        {
            LogDbg("Send statbuff length:%d", len);
        }
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_CMD_TYPE_CHANGE_TPOOL_TIMEOUT].Opt))
    {
        uint32_t timeout = (uint32_t)atoi(strchr(Buff, ' '));
        sprintf(retString, "Set tpool timeout as %u.", timeout);
        len = send(Fd, retString, strlen(retString) + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        TPoolSetTimeout(timeout);
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_CMD_TYPE_CHANGE_TPOOL_QUEUE_LENGTH].Opt))
    {
        int32_t queueLength = (int32_t)atoi(strchr(Buff, ' '));
        sprintf(retString, "Set tpool queue length as %u.", queueLength);
        len = send(Fd, retString, strlen(retString) + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        TPoolSetMaxQueueLength(queueLength);
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_CMD_TYPE_CHANGE_LOG_LEVEL].Opt))
    {
        uint32_t logLevel = (uint32_t)atoi(strchr(Buff, ' '));
        sprintf(retString, "Set log level as %u.", logLevel);
        len = send(Fd, retString, strlen(retString) + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        LogSetLevel(logLevel);
    }
    else
    {
        MY_UATOMIC_DEC(&sg_CmdLineWorker.Stat.ExecCnt);
        ret = MY_ENOSYS;
    }
    LogDbg("%d:%s", ret, My_StrErr(ret));
    return ret;
}

void*
_CmdServer_WorkerFunc(
    void* arg
    )
{
    int ret = 0;
    int serverFd = -1;
    int epollFd = -1;
    int event_count = 0;
    struct epoll_event event, waitEvents[MY_BUFF_128];
    int loop = 0;
    int clientFd[MY_MAX_CLIENT_NUM_PER_SERVER] = {0};
    int clientFdCnt = 0;
    char recvBuff[MY_BUFF_128] = {0};
    int recvLen = 0;

    ret = _CmdServer_Init(&serverFd, &epollFd);
    if (ret)
    {
        LogErr("Cmd server init failed %d", ret);
        fflush(stdout);
        return NULL;
    }

    UNUSED(arg);
    /* recv */
    while (!sg_CmdLineWorker.Exit)
    {
        event_count = epoll_wait(epollFd, waitEvents, MY_BUFF_128, 100); //timeout 0.1s
        if (event_count == -1)
        {
            LogErr("Epoll wait failed! (%d:%s)", errno, My_StrErr(errno));
            goto CommonReturn;
        }
        else if (event_count == 0)
        {
            continue;
        }
        for(loop = 0; loop < event_count; loop ++)
        {
            if (waitEvents[loop].data.fd == serverFd)
            {
                int tmpClientFd = -1;
                struct sockaddr tmpClientaddr;
                socklen_t tmpClientLen;
                MY_UATOMIC_INC(&sg_CmdLineWorker.Stat.ConnectCnt);
                tmpClientFd = accept(serverFd, &tmpClientaddr, &tmpClientLen);
                if (tmpClientFd != -1)
                {
                    int flags = fcntl(tmpClientFd, F_GETFL, 0);
                    fcntl(tmpClientFd, F_SETFL, flags | O_NONBLOCK);    // set non block

                    event.data.fd = tmpClientFd;
                    event.events = EPOLLIN | EPOLLET;  
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, tmpClientFd, &event);
                    clientFd[clientFdCnt ++] = tmpClientFd;
                }
            }
            else if (waitEvents[loop].events & EPOLLIN)
            {
                memset(recvBuff, 0, sizeof(recvBuff));
                recvLen = recv(waitEvents[loop].data.fd, recvBuff, sizeof(recvBuff), 0);
                if (recvLen > 0)
                {
                    LogInfo("Recv cmdline msg [%s]", recvBuff);
                    ret = _CmdServerHandleMsg(waitEvents[loop].data.fd, recvBuff);
                    if (ret)
                    {
                        LogErr("Handle msg filed %d", ret);
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                        close(waitEvents[loop].data.fd);
                    }
                }
                else if (recvLen == 0)
                {
                    LogInfo("Client closed connection.");
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                    close(waitEvents[loop].data.fd);
                }
                else
                {
                    ret = MY_EIO;
                    LogErr("Recv in %d failed %d, errno %s:%d", waitEvents[loop].data.fd, recvLen, errno, strerror(errno));
                    goto CommonReturn;
                }
            }
            else if (waitEvents[loop].events & EPOLLERR || waitEvents[loop].events & EPOLLHUP)
            {
                LogInfo("%d error happen!", waitEvents[loop].data.fd);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                close(waitEvents[loop].data.fd);
                continue;
            }
        }
    }
    
CommonReturn:
    for(loop = 0; loop < clientFdCnt; loop ++)
    {
        if (clientFd[loop] > 0)
            close(clientFd[loop]);
    }
    if (serverFd > 0)
        close(serverFd);
    if (epollFd > 0)
        close(epollFd);
    return NULL;
}

static
BOOL 
_CmdLineIsSupported(
    char* Cmd
    )
{
    int loop = 0;
    if (!Cmd)
    {
        return FALSE;
    }
    
    for(loop = 0; loop < MY_CMD_TYPE_MAX_UNUSED; loop ++)
    {
        if (strstr(Cmd, sg_CmdLineCont[loop].Opt) == 0)
            return TRUE;
    }

    return FALSE;
}

int
_CmdClient_WorkerFunc(
    char* Cmd
    )
{
    int ret = 0;
    int clientFd = -1;
    unsigned int serverIp = 0;
    struct sockaddr_in serverAddr = {0};
    int32_t reuseable = 1; // set port reuseable when fd closed

    if (!Cmd || !_CmdLineIsSupported(Cmd))
    {
        ret = MY_EINVAL;
        _CmdLineUsage(NULL);
        goto CommonReturn;
    }
    if (strcasecmp(sg_CmdLineCont[MY_CMD_TYPE_START].Opt, Cmd) == 0)
    {
        printf("Already running!\n");
        goto CommonReturn;
    }

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > clientFd)
    {
        printf("Create socket failed\n");
        goto CommonReturn;
    }
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MY_SERVER_CMD_LINE_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverIp);
    serverAddr.sin_addr.s_addr = serverIp;
    if(0 > connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
    {
        printf("Connect failed\n");
        goto CommonReturn;
    }

    ret = send(clientFd, Cmd, strlen(Cmd) + 1, 0);
    if (ret > 0)
    {
        ret = 0;
    }
    else
    {
        ret = errno;
        printf("Send cmdline failed, %d %s\n", ret, My_StrErr(ret));
        goto CommonReturn;
    }

    char recvBuff[MY_BUFF_1024 * MY_BUFF_1024] = {0};
    ret = recv(clientFd, recvBuff, sizeof(recvBuff), 0);
    if (ret > 0)
    {
        ret = 0;
        printf("%s\n", recvBuff);
    }
    else
    {
        ret = errno;
        printf("Recv cmdline reply failed, %d %s\n", ret, My_StrErr(ret));
        goto CommonReturn;
    }

CommonReturn:
    return ret;
}

int
CmdLineModuleInit(
    MY_CMDLINE_MODULE_INIT_ARG *InitArg
    )
{
    int ret = 0;
    int pidFd = -1;
    char path[MY_BUFF_64] = {0};
    int isRunning = 0;
    char cmd[MY_BUFF_64] = {0};
    
    if (!InitArg || (InitArg->Argc != 2 && InitArg->Argc != 3) || 
        !InitArg->Argv || !InitArg->RoleName || !InitArg->ExitFunc)
    {
        goto CommonErr;
    }

    sg_CmdLineWorker.ExitCb = InitArg->ExitFunc;
    sprintf(path, "/tmp/%s.pid", InitArg->RoleName);
    pidFd = MyUtil_OpenPidFile(path);
    if (pidFd < 0)
    {
        ret = MY_EIO;
        goto CommonReturn;
    }
    isRunning = MyUtil_IsProcessRunning(pidFd);
    if (isRunning == -1)
    {
        ret = MY_EIO;
        goto CommonReturn;
    }
    sg_CmdLineWorker.Role = isRunning ? MY_CMDLINE_ROLE_CLT : MY_CMDLINE_ROLE_SVR;
    
    if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_CMD_TYPE_HELP].Opt) == 0)
    {
        goto CommonErr;
    }
    else if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_CMD_TYPE_START].Opt) != 0 && 
            MY_CMDLINE_ROLE_SVR == sg_CmdLineWorker.Role)
    {
        if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_CMD_TYPE_STOP].Opt) == 0)
        {
            LogErr("%s is not running!", InitArg->RoleName);
        }
        goto CommonErr;
    }

    switch (sg_CmdLineWorker.Role)
    {
        case MY_CMDLINE_ROLE_SVR:
            MyUtil_MakeDaemon();
            ret = MyUtil_SetPidIntoFile(pidFd);
            if (ret)
            {
                goto CommonReturn;
            }
            MyUtil_CloseStdFds();
            /* start worker pthread */
            sg_CmdLineWorker.Thread = (pthread_t*)calloc(sizeof(pthread_t), 1);
            if (!sg_CmdLineWorker.Thread)
            {
                ret = MY_ENOMEM;
                LogErr("Apply memory failed!");
                goto CommonReturn;
            }
            sg_CmdLineWorker.Exit = FALSE;
            ret = pthread_create(sg_CmdLineWorker.Thread, NULL, _CmdServer_WorkerFunc, NULL);
            if (ret) 
            {
                LogErr("Failed to create thread");
                goto CommonReturn;
            }
            break;
        case MY_CMDLINE_ROLE_CLT:
            if (InitArg->Argc == 2)
            {
                sprintf(cmd, "%s", InitArg->Argv[1]);
            }
            else if (InitArg->Argc == 3)
            {
                sprintf(cmd, "%s %s", InitArg->Argv[1], InitArg->Argv[2]);
            }
            (void)_CmdClient_WorkerFunc(cmd);
            ret = MY_ERR_EXIT_WITH_SUCCESS;
            goto CommonReturn;
        default:
            ret = EINVAL;
            LogErr("Role %d invalid!", sg_CmdLineWorker.Role);
            break;
    }
    goto CommonReturn;

CommonErr:
    _CmdLineUsage(InitArg->RoleName);
    ret = MY_ERR_EXIT_WITH_SUCCESS;
CommonReturn:
    return ret;
}

void
CmdLineModuleExit(
    void
    )
{
    if (!sg_CmdLineWorker.Exit)
    {
        sg_CmdLineWorker.Exit = TRUE;
        if (sg_CmdLineWorker.Thread)
        {
            pthread_join(*sg_CmdLineWorker.Thread, NULL);
            free(sg_CmdLineWorker.Thread);
            sg_CmdLineWorker.Thread = NULL;
        }
    }
}

int
CmdLineModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = MY_SUCCESS;

    *Offset += snprintf(Buff + *Offset , BuffMaxLen - *Offset, 
            "<%s:[IsRunning:%s CmdConnectCnt:%d CmdExecCnt:%d]>", ModuleNameByEnum(MY_MODULES_ENUM_CMDLINE), 
                sg_CmdLineWorker.Exit ? "FALSE" : "TRUE", sg_CmdLineWorker.Stat.ConnectCnt, sg_CmdLineWorker.Stat.ExecCnt); 
    return ret;
}

