#include "include.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myModuleCommon.h"

#define MY_TEST_MAX_EVENTS                                      1024
#define MY_TEST_SERVER_ROLE_NAME                                "tcpserver"

static int sg_MsgId = 0;

pthread_t *ServerMsgHandler = NULL;

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
    localAddr.sin_port = htons(MY_TEST_TCP_SERVER_PORT);
    localAddr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
    {
        ret = -1;
        LogErr("Bind failed");
        goto CommonReturn;
    }
    LogInfo("Bind serverFd: %d", serverFd);

    if(0 > listen(serverFd, MY_TEST_MAX_CLIENT_NUM_PER_SERVER))
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
    MY_TEST_MSG Msg
    )
{
    int ret = 0;
    MY_TEST_MSG *replyMsg = NewMsg();
    struct timeval tv;

    if (!replyMsg)
    {
        ret = -1;
        goto CommonReturn;
    }
    if (strcasecmp((char*)Msg.Cont.VarLenCont, MY_TEST_DISCONNECT_STRING) == 0)
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
    MY_TEST_MSG *msg = NULL;
    UNUSED(arg);

    serverFd = _Server_CreateFd();
    if (0 > serverFd)
    {
        LogErr("Create server socket failed");
        goto CommonReturn;
    }

    int epoll_fd = -1;
    int event_count = 0;
    struct epoll_event event, waitEvents[MY_TEST_MAX_EVENTS];

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
    while (1)
    {
        event_count = epoll_wait(epoll_fd, waitEvents, MY_TEST_MAX_EVENTS, 1000); 
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
                BOOL isPeerClosed = FALSE;
                msg = RecvMsg(waitEvents[loop].data.fd, &isPeerClosed);
                if (msg)
                {
                    ret = _Server_HandleMsg(waitEvents[loop].data.fd, *msg);
                    if (ret)
                    {
                        LogErr("Handle msg filed %d", ret);
                        break;
                    }
                }
                else
                {
                    if (isPeerClosed)
                    {
                        LogInfo("Peer Socket exit: %d", waitEvents[loop].data.fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                        close(waitEvents[loop].data.fd);
                    }
                    else
                    {
                        LogErr("Recv from %d failed %d", waitEvents[loop].data.fd, ret);
                    }
                }
            }
            else if (waitEvents[loop].events & EPOLLERR || waitEvents[loop].events & EPOLLHUP)
            {
                LogInfo("%d error happen!", waitEvents[loop].data.fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                close(waitEvents[loop].data.fd);
                continue;
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
_Server_Init(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    MY_MODULES_INIT_PARAM initParam;
    memset(&initParam, 0, sizeof(initParam));

    initParam.Argc = argc;
    initParam.Argv = argv;
    initParam.LogFile = MY_TEST_LOG_FILE;
    initParam.RoleName = MY_TEST_SERVER_ROLE_NAME;
    initParam.TPoolSize = 10;
    initParam.TPoolTimeout = 5;
    ret = MyModuleCommonInit(initParam);
    if (ret)
    {
        LogErr("MyModuleCommonInit failed!");
        goto CommonReturn;
    }
    
    if (!ServerMsgHandler)
    {
        ServerMsgHandler = (pthread_t*)malloc(sizeof(pthread_t));
        if (!ServerMsgHandler)
        {
            ret = MY_ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        ret = pthread_create(ServerMsgHandler, NULL, _Server_WorkerFunc, NULL);
        if (ret) 
        {
            LogErr("Failed to create thread");
            goto CommonReturn;
        }
    }
    LogInfo("Server init success!");
CommonReturn:
    return ret;
}

void
Server_Exit(
    void
    )
{
    // msg handler
    if (ServerMsgHandler)
    {
        free(ServerMsgHandler);
        ServerMsgHandler = NULL;
    }
    LogInfo("----------------- MsgHandler exited! -------------------");

    MyModuleCommonExit();
    system("killall "MY_TEST_SERVER_ROLE_NAME);
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
        LogErr("Server init failed! ret %d", ret);
        goto CommonReturn;
    }
    
    LogInfo("Joining thread: Server Worker Func");
    pthread_join(*ServerMsgHandler, NULL);
    
CommonReturn:
    //Server_Exit();
    return ret;
}
