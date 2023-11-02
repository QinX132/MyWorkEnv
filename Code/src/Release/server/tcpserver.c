#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myModuleCommon.h"

#define MY_MAX_EVENTS                                       1024
#define MY_SERVER_ROLE_NAME                                 "tcpserver"
#define MY_SERVER_CONF_ROOT                                 MY_SERVER_ROLE_NAME".conf"

typedef struct {
    MY_LOG_LEVEL LogLevel;
    char LogFilePath[MY_BUFF_64];
}
SERVER_CONF_PARAM;

static int sg_MsgId = 0;

pthread_t *sg_ServerMsgHandler = NULL;
static BOOL sg_ServerMsgHandlerShouldExit = FALSE;

static int
_Server_CreateFd(
    void
    )
{
    int ret = 0;
    int serverFd = -1;
    serverFd = socket(AF_INET, SOCK_STREAM, 0);     //create socket

    int32_t reuseable = 1; // set port reuseable when fd closed
    (void)setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));    // set reuseable

    int nonBlock = fcntl(serverFd, F_GETFL, 0);
    nonBlock |= O_NONBLOCK;
    fcntl(serverFd, F_SETFL, nonBlock);         // set fd nonBlock

    struct sockaddr_in localAddr = {0};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(MY_TCP_SERVER_PORT);
    localAddr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
    {
        ret = -1;
        LogErr("Bind failed");
        goto CommonReturn;
    }
    LogInfo("Bind serverFd: %d", serverFd);

    if(0 > listen(serverFd, MY_MAX_CLIENT_NUM_PER_SERVER))
    {
        ret = -1;
        LogErr("Listen failed");
        goto CommonReturn;
    }

CommonReturn:
    if (ret && serverFd != -1)
    {
        close(serverFd);
        serverFd = -1;
    }
    return serverFd;
}

static
int 
_Server_HandleMsg(
    int Fd,
    MY_MSG Msg
    )
{
    int ret = 0;
    MY_MSG *replyMsg = NewMsg();
    struct timeval tv;

    if (!replyMsg)
    {
        ret = -1;
        goto CommonReturn;
    }
    if (strcasecmp((char*)Msg.Cont.VarLenCont, MY_DISCONNECT_STRING) == 0)
    {
        ret = -1;
        goto CommonReturn;
    }

    gettimeofday(&tv, NULL);
    replyMsg->Head.MsgId = sg_MsgId ++;
    replyMsg->Head.MsgContentLen = sizeof("reply");
    replyMsg->Head.MagicVer = Msg.Head.MagicVer;
    replyMsg->Head.SessionId = Msg.Head.SessionId;
    memcpy(replyMsg->Cont.VarLenCont, "reply", sizeof("reply"));
    replyMsg->Tail.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;

    ret = SendMsg(Fd, *replyMsg);
    if (ret)
    {
        LogErr("Send msg failed");
    }

CommonReturn:
    if (replyMsg)
    {
        FreeMsg(replyMsg);
    }
    return ret;
}

static void*
_Server_WorkerFunc(
    void* arg
    )
{
    int serverFd = -1;
    int ret = 0;
    MY_MSG msg;
    UNUSED(arg);

    serverFd = _Server_CreateFd();
    if (0 > serverFd)
    {
        LogErr("Create server socket failed");
        goto CommonReturn;
    }

    int epoll_fd = -1;
    int event_count = 0;
    struct epoll_event event, waitEvents[MY_MAX_EVENTS];

    epoll_fd = epoll_create1(0);
    if (0 > epoll_fd)
    {
        LogErr("Create epoll socket failed %d", errno);
        goto CommonReturn;
    }
    event.events = EPOLLIN; // LT fd
    event.data.fd = serverFd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverFd, &event))
    {
        LogErr("Add to epoll socket failed %d", errno);
        goto CommonReturn;
    }

    int loop = 0;
    /* recv */
    while (!sg_ServerMsgHandlerShouldExit)
    {
        event_count = epoll_wait(epoll_fd, waitEvents, MY_MAX_EVENTS, 1000); 
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
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tmpClientFd, &event);
                }
            }
            else if (waitEvents[loop].events & EPOLLIN)
            {
                ret = RecvMsg(waitEvents[loop].data.fd, &msg);
                if (!ret)
                {
                    ret = _Server_HandleMsg(waitEvents[loop].data.fd, msg);
                    if (ret)
                    {
                        LogErr("Handle msg filed %d", ret);
                        break;
                    }
                }
                else
                {
                    if (MY_ERR_PEER_CLOSED == ret)
                    {
                        LogErr("Peer closed , fd = %d", waitEvents[loop].data.fd);
                    }
                    else
                    {
                        LogErr("Recv from %d failed %d:%s", waitEvents[loop].data.fd, ret, My_StrErr(ret));
                    }
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                    close(waitEvents[loop].data.fd);
                }
            }
            else if (waitEvents[loop].events & EPOLLERR || waitEvents[loop].events & EPOLLHUP)
            {
                LogInfo("%d error happen!", waitEvents[loop].data.fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                close(waitEvents[loop].data.fd);
            }
        }
    }

CommonReturn:
    if (serverFd != -1)
        close(serverFd);
    if (epoll_fd != -1)
        close(epoll_fd);
    
    return NULL;
}

static int 
_Server_ParseConf(
    SERVER_CONF_PARAM *ServerConf
    )
{
    int ret = 0;
    FILE *fp = NULL;
    char line[MY_BUFF_128] = {0};
    char *ptr = NULL;
    int len = 0;

    fp = fopen(MY_SERVER_CONF_ROOT, "r");
    if (!fp)
    {
        ret = MY_EIO;
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
                ret = MY_EIO;
                LogErr("Invalid loglevel!");
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "LogPath=")) != NULL)
        {
            len = snprintf(ServerConf->LogFilePath, sizeof(ServerConf->LogFilePath), "%s", ptr + strlen("LogPath="));
            if (!len)
            {
                ret = MY_EIO;
                LogErr("Invalid logPath %s!", ptr + strlen("LogPath="));
                goto CommonReturn;
            }
            if ('\n' == ServerConf->LogFilePath[len - 1])
            {
                ServerConf->LogFilePath[len - 1] = '\0';
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
_Server_Exit(
    void
    )
{
    // msg handler
    if (sg_ServerMsgHandler)
    {
        sg_ServerMsgHandlerShouldExit = TRUE;
        pthread_join(*sg_ServerMsgHandler, NULL);
        MyFree(sg_ServerMsgHandler);
    }
    LogInfo("----------------- MsgHandler exited! -------------------");

    MyModuleCommonExit();
    system("killall "MY_SERVER_ROLE_NAME);
}

static int
_Server_Init(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    MY_MODULES_INIT_PARAM initParam;
    SERVER_CONF_PARAM serverConf;
    
    memset(&initParam, 0, sizeof(initParam));
    memset(&serverConf, 0, sizeof(serverConf));

    ret = _Server_ParseConf(&serverConf);
    if (ret)
    {
        LogErr("Init conf failed!");
        goto CommonReturn;
    }

    initParam.InitHealthModule = TRUE;
    initParam.InitMsgModule = TRUE;
    initParam.InitTimerModule = TRUE;
    initParam.CmdLineArg = (MY_CMDLINE_MODULE_INIT_ARG*)calloc(sizeof(MY_CMDLINE_MODULE_INIT_ARG), 1);
    initParam.LogArg = (MY_LOG_MODULE_INIT_ARG*)calloc(sizeof(MY_LOG_MODULE_INIT_ARG), 1);
    initParam.TPoolArg = (MY_TPOOL_MODULE_INIT_ARG*)calloc(sizeof(MY_TPOOL_MODULE_INIT_ARG), 1);
    if (!initParam.CmdLineArg || !initParam.LogArg || !initParam.TPoolArg)
    {
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    // cmd line init args
    initParam.CmdLineArg->Argc = argc;
    initParam.CmdLineArg->Argv = argv;
    initParam.CmdLineArg->RoleName = MY_SERVER_ROLE_NAME;
    initParam.CmdLineArg->ExitFunc = _Server_Exit;
    // log init args
    initParam.LogArg->LogFilePath = serverConf.LogFilePath;
    initParam.LogArg->LogLevel = serverConf.LogLevel;
    initParam.LogArg->RoleName = MY_SERVER_ROLE_NAME;
    // tpool init args
    initParam.TPoolArg->ThreadPoolSize = 5;
    initParam.TPoolArg->Timeout = 5;
    ret = MyModuleCommonInit(initParam);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("MyModuleCommonInit failed!");
        }
        goto CommonReturn;
    }
    
    if (!sg_ServerMsgHandler)
    {
        sg_ServerMsgHandler = (pthread_t*)MyCalloc(sizeof(pthread_t));
        if (!sg_ServerMsgHandler)
        {
            ret = MY_ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        sg_ServerMsgHandlerShouldExit = FALSE;
        ret = pthread_create(sg_ServerMsgHandler, NULL, _Server_WorkerFunc, NULL);
        if (ret) 
        {
            LogErr("Failed to create thread");
            goto CommonReturn;
        }
    }
    LogInfo("Server init success!");
CommonReturn:
    if (initParam.LogArg)
    {
        free(initParam.LogArg);
    }
    if (initParam.CmdLineArg)
    {
        free(initParam.CmdLineArg);
    }
    if (initParam.TPoolArg)
    {
        free(initParam.TPoolArg);
    }
    return ret;
}

int ServerHealthMonitor(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = 0;
    int len = 0;
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, "<%s:[%s]>", "Server", "Active");
    *Offset += len;

    return ret;
}

void ServerTPoolCb(
    void* Arg
    )
{
    UNUSED(Arg);
    LogInfo("Tpool task Callback entered");
    sleep(10);
}

int
main(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    ret = _Server_Init(argc, argv);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("Server init failed! ret %d", ret);
        }
        goto CommonReturn;
    }

/*
    while(1)
    {
        sleep(1);
        ret = AddTaskIntoThread(ServerTPoolCb, NULL);
        if (ret)
        {
            LogErr("Add task failed ret %d", ret);
            break;
        }
        ret = AddTaskIntoThreadAndWait(ServerTPoolCb, NULL);
        if (ret)
        {
            LogErr("Add task failed ret %d", ret);
            break;
        }
    }
*/
    while(1)
    {
        sleep(1);
    }
CommonReturn:
    //Server_Exit();
    return ret;
}
