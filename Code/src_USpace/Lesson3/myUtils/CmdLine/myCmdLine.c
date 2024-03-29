#define _GNU_SOURCE

#include "myLogIO.h"
#include "myCmdLine.h"
#include "myCommonUtil.h"
#include "myModuleHealth.h"

static MY_TEST_CMDLINE_ROLE sg_CmdLineRole = MY_TEST_CMDLINE_ROLE_UNUSED;
static pthread_t *sg_CmdLineWorker = NULL;
static ExitHandle sg_ExitHandle = NULL;

#define MY_TEST_CMDLINE_ARG_LIST                 \
        __MY_TEST_ARG("start", "start this program")  \
        __MY_TEST_ARG("showstat", "show this program's modules stats")  \
        __MY_TEST_ARG("stop", "stop this program")  \
        __MY_TEST_ARG("help", "show this page")  \
        __MY_TEST_ARG("changeTPoolTimeout", "<Timout> (second)")  \
        __MY_TEST_ARG("changeLogLevel", "<Level> (0-info 1-debug 2-warn 3-error)")

static const MY_TEST_CMDLINE_CONT sg_CmdLineCont[MY_TEST_CMD_TYPE_UNUSED] = 
{
    #undef __MY_TEST_ARG
    #define __MY_TEST_ARG(_opt_,_help_) \
        {_opt_, _help_},
    MY_TEST_CMDLINE_ARG_LIST
};

void
CmdLineUsage(
    char* RoleName
    )
{
    printf("--------------------------------------------------------------------------\n");
    printf("%10s Usage:\n\n", RoleName ? RoleName : "CmdLine");
    #undef __MY_TEST_ARG
    #define __MY_TEST_ARG(_opt_,_help_) \
        printf("%20s: [%-30s]\n", _opt_, _help_);
    MY_TEST_CMDLINE_ARG_LIST
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
    localAddr.sin_port = htons(MY_TEST_SERVER_CMD_LINE_PORT);
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
    char retString[MY_TEST_BUFF_128] = {0};
    
    if (strcasestr(Buff, sg_CmdLineCont[MY_TEST_CMD_TYPE_STOP].Opt))
    {
        len = send(Fd, "Process stopped.", sizeof("Process stopped."), 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        if (sg_ExitHandle)
        {
            sg_ExitHandle();
        }
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_TEST_CMD_TYPE_SHOWSTAT].Opt))
    {
        char statBuff[MY_TEST_BUFF_1024] = {0};
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
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_TEST_CMD_TYPE_CHANGE_TPOOL_TIMEOUT].Opt))
    {
        uint32_t timeout = (uint32_t)atoi(strchr(Buff, ' '));
        sprintf(retString, "Set tpool timeout as %u.", timeout);
        len = send(Fd, retString, strlen(retString) + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        SetTPoolTimeout(timeout);
    }
    else if (strcasestr(Buff, sg_CmdLineCont[MY_TEST_CMD_TYPE_CHANGE_LOG_LEVEL].Opt))
    {
        uint32_t logLevel = (uint32_t)atoi(strchr(Buff, ' '));
        sprintf(retString, "Set log level as %u.", logLevel);
        len = send(Fd, retString, strlen(retString) + 1, 0);
        if (len <= 0)
        {
            ret = MY_EIO;
            LogErr("Send CmdLine reply failed!");
        }
        SetLogLevel(logLevel);
    }
    else
    {
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
    struct epoll_event event, waitEvents[MY_TEST_BUFF_128];
    int loop = 0;
    int clientFd[MY_TEST_MAX_CLIENT_NUM_PER_SERVER] = {0};
    int clientFdCnt = 0;
    char recvBuff[MY_TEST_BUFF_128] = {0};
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
    while (1)
    {
        event_count = epoll_wait(epollFd, waitEvents, MY_TEST_BUFF_128, 1000); //timeout 1s
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
                    LogErr("Recv in %d failed %d", waitEvents[loop].data.fd, recvLen);
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
    
    for(loop = 0; loop < MY_TEST_CMD_TYPE_UNUSED; loop ++)
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
        CmdLineUsage(NULL);
        goto CommonReturn;
    }
    if (strcasecmp(sg_CmdLineCont[MY_TEST_CMD_TYPE_START].Opt, Cmd) == 0)
    {
        LogErr("Already running!");
        goto CommonReturn;
    }

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > clientFd)
    {
        LogErr("Create socket failed");
        goto CommonReturn;
    }
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MY_TEST_SERVER_CMD_LINE_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverIp);
    serverAddr.sin_addr.s_addr = serverIp;
    if(0 > connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
    {
        LogErr("Connect failed");
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
        LogErr("Send cmdline failed, %d %s", ret, My_StrErr(ret));
        goto CommonReturn;
    }

    char recvBuff[MY_TEST_BUFF_1024] = {0};
    ret = recv(clientFd, recvBuff, sizeof(recvBuff), 0);
    if (ret > 0)
    {
        ret = 0;
        LogErr("%s", recvBuff);
    }
    else
    {
        ret = errno;
        LogErr("Recv cmdline reply failed, %d %s", ret, My_StrErr(ret));
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
    char path[MY_TEST_BUFF_64] = {0};
    int isRunning = 0;
    char cmd[MY_TEST_BUFF_64] = {0};
    
    if (!InitArg || (InitArg->Argc != 2 && InitArg->Argc != 3) || !InitArg->Argv || !InitArg->RoleName || !InitArg->ExitFunc)
    {
        goto CommonErr;
    }

    sg_ExitHandle = InitArg->ExitFunc;
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
    sg_CmdLineRole = isRunning ? MY_TEST_CMDLINE_ROLE_CLT : MY_TEST_CMDLINE_ROLE_SVR;
    
    if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_TEST_CMD_TYPE_HELP].Opt) == 0)
    {
        goto CommonErr;
    }
    else if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_TEST_CMD_TYPE_START].Opt) != 0 && 
            MY_TEST_CMDLINE_ROLE_SVR == sg_CmdLineRole)
    {
        if (strcasecmp(InitArg->Argv[1], sg_CmdLineCont[MY_TEST_CMD_TYPE_STOP].Opt) == 0)
        {
            LogErr("%s is not running!", InitArg->RoleName);
        }
        goto CommonErr;
    }

    switch (sg_CmdLineRole)
    {
        case MY_TEST_CMDLINE_ROLE_SVR:
            MyUtil_MakeDaemon();
            ret = MyUtil_SetPidIntoFile(pidFd);
            if (ret)
            {
                goto CommonReturn;
            }
            MyUtil_CloseStdFds();
            /* start worker pthread */
            sg_CmdLineWorker = (pthread_t*)MyCalloc(sizeof(pthread_t));
            if (!sg_CmdLineWorker)
            {
                ret = MY_ENOMEM;
                LogErr("Apply memory failed!");
                goto CommonReturn;
            }
            ret = pthread_create(sg_CmdLineWorker, NULL, _CmdServer_WorkerFunc, NULL);
            if (ret) 
            {
                LogErr("Failed to create thread");
                goto CommonReturn;
            }
            break;
        case MY_TEST_CMDLINE_ROLE_CLT:
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
            LogErr("Role %d invalid!", sg_CmdLineRole);
            break;
    }
    goto CommonReturn;

CommonErr:
    CmdLineUsage(InitArg->RoleName);
    ret = MY_ERR_EXIT_WITH_SUCCESS;
CommonReturn:
    return ret;
}

void
CmdLineModuleExit(
    void
    )
{
    MyFree(sg_CmdLineWorker);
}
