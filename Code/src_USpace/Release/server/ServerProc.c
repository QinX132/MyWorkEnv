#include "include.h"
#include "myModuleCommon.h"
#include "ServerClientMap.h"
#include "ServerMsgHandler.h"

#define MY_SERVER_MAX_EVENTS                                1024

typedef struct _SERVER_PROC_WORKER
{
    int ServerFd;
    int EpollFd;
    pthread_t Thread;
    BOOL Running;
    
    SERVER_CLIENT_MAP ClientMap;
}
SERVER_PROC_WORKER;

static SERVER_PROC_WORKER sg_ServerProcWorker = {.ServerFd = -1, .Running = FALSE};

static int
_ServerCreateFd(
    void
    )
{
    int ret = MY_SUCCESS;
    sg_ServerProcWorker.ServerFd = socket(AF_INET, SOCK_STREAM, 0);     //create socket
    LogInfo("Open serverFd %d", sg_ServerProcWorker.ServerFd);

    int32_t reuseable = 1; // set port reuseable when fd closed
    (void)setsockopt(sg_ServerProcWorker.ServerFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));    // set reuseable

    int nonBlock = fcntl(sg_ServerProcWorker.ServerFd, F_GETFL, 0);
    nonBlock |= O_NONBLOCK;
    fcntl(sg_ServerProcWorker.ServerFd, F_SETFL, nonBlock);         // set fd nonBlock

    struct sockaddr_in localAddr = {0};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(MY_TCP_SERVER_PORT);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sg_ServerProcWorker.ServerFd, (void *)&localAddr, sizeof(localAddr)))
    {
        ret = -errno;
        LogErr("Bind failed");
        goto CommonReturn;
    }
    LogInfo("Bind serverFd: %d, port:%u ", sg_ServerProcWorker.ServerFd, MY_TCP_SERVER_PORT);

    if(listen(sg_ServerProcWorker.ServerFd, MY_MAX_CLIENT_NUM_PER_SERVER))
    {
        ret = -errno;
        LogErr("Listen failed");
        goto CommonReturn;
    }

CommonReturn:
    if (ret && sg_ServerProcWorker.ServerFd != -1)
    {
        LogErr("ret %d:%s", ret, My_StrErr(ret));
        close(sg_ServerProcWorker.ServerFd);
        sg_ServerProcWorker.ServerFd = -1;
    }
    return ret;
}

static int
_ServerAcceptNewConnect(
    int EpollFd
    )
{
    int ret = MY_SUCCESS;
    int clientFd = -1;
    struct sockaddr tmpClientaddr;
    socklen_t tmpClientLen;
    int flags = 0;
    struct epoll_event event;

    memset(&event, 0, sizeof(event));
    
    clientFd = accept(sg_ServerProcWorker.ServerFd, &tmpClientaddr, &tmpClientLen);
    if (clientFd != -1)
    {
        flags = fcntl(clientFd, F_GETFL, 0);
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);    // set non block
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

        event.data.fd = clientFd;
        event.events = EPOLLIN | EPOLLET;  
        epoll_ctl(EpollFd, EPOLL_CTL_ADD, clientFd, &event);
        LogInfo("Recv new connection, fd %d", clientFd);
        ret = ServerClientMapAddNode(&sg_ServerProcWorker.ClientMap, clientFd);
    }

    return ret;
}

static void*
_ServerProcFn(
    void* arg
    )
{
    int ret = MY_SUCCESS;
    MY_MSG msg;
    UNUSED(arg);

    ret = _ServerCreateFd();
    if (ret || sg_ServerProcWorker.ServerFd == -1)
    {
        LogErr("Create server socket failed");
        goto CommonReturn;
    }

    int epollFd = -1;
    int event_count = 0;
    struct epoll_event event, waitEvents[MY_SERVER_MAX_EVENTS];

    epollFd = epoll_create1(0);
    if (0 > epollFd)
    {
        LogErr("Create epoll socket failed %d", errno);
        goto CommonReturn;
    }
    event.events = EPOLLIN; // LT fd
    event.data.fd = sg_ServerProcWorker.ServerFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, sg_ServerProcWorker.ServerFd, &event))
    {
        LogErr("Add to epoll socket failed %d", errno);
        goto CommonReturn;
    }

    int loop = 0;
    /* recv */
    while (sg_ServerProcWorker.Running)
    {
        event_count = epoll_wait(epollFd, waitEvents, MY_SERVER_MAX_EVENTS, 100); 
        for(loop = 0; loop < event_count; loop ++)
        {
            if (waitEvents[loop].data.fd == sg_ServerProcWorker.ServerFd)
            {
                ret = _ServerAcceptNewConnect(epollFd);
                if (ret)
                {
                    LogDbg("Accept new connect failed! ret %d:%s", ret, My_StrErr(ret));
                }
            }
            else if (waitEvents[loop].events & EPOLLIN)
            {
                ret = RecvMsg(waitEvents[loop].data.fd, &msg);
                if (ret == MY_SUCCESS)
                {
                    ret = ServerHandleMsg(waitEvents[loop].data.fd, &msg);
                    if (ret)
                    {
                        LogDbg("Handle msg filed %d", ret);
                    }
                }
                else
                {
                    if (-MY_ERR_PEER_CLOSED == ret)
                    {
                        LogDbg("Peer closed , fd = %d", waitEvents[loop].data.fd);
                    }
                    else
                    {
                        LogErr("Recv from %d failed %d:%s", waitEvents[loop].data.fd, ret, My_StrErr(ret));
                    }
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                    close(waitEvents[loop].data.fd);
                    ServerClientMapDelByFd(&sg_ServerProcWorker.ClientMap, waitEvents[loop].data.fd);
                }
            }
            else if (waitEvents[loop].events & EPOLLERR || waitEvents[loop].events & EPOLLHUP)
            {
                LogInfo("%d error happen!", waitEvents[loop].data.fd);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                close(waitEvents[loop].data.fd);
                ServerClientMapDelByFd(&sg_ServerProcWorker.ClientMap, waitEvents[loop].data.fd);
            }
        }
    }

CommonReturn:
    if (sg_ServerProcWorker.ServerFd != -1)
    {
        close(sg_ServerProcWorker.ServerFd);
        sg_ServerProcWorker.ServerFd = -1;
    }
    if (epollFd != -1)
        close(epollFd);
    
    return NULL;
}

int
ServerProcInit(
    void
    )
{
    int ret = MY_SUCCESS;
    
    if (!sg_ServerProcWorker.Running)
    {
        sg_ServerProcWorker.Running = TRUE;
        ServerClientMapInit(&sg_ServerProcWorker.ClientMap);
        ret = pthread_create(&sg_ServerProcWorker.Thread, NULL, _ServerProcFn, NULL);
        if (ret) 
        {
            LogErr("Failed to create thread");
            goto CommonReturn;
        }
    }
    LogInfo("ServerProc init success!");

CommonReturn:
    return ret;
}

void
ServerProcExit(
    void
    )
{
    if (sg_ServerProcWorker.Running)
    {
        sg_ServerProcWorker.Running = FALSE;
        pthread_join(sg_ServerProcWorker.Thread, NULL);
        ServerClientMapExit(&sg_ServerProcWorker.ClientMap);
    }
}

int
ServerProcCmdShowStats(
    __in char* Cmd,
    __in size_t CmdLen,
    __out char* ReplyBuff,
    __out size_t ReplyBuffLen
    )
{
    int offset = 0;
    UNUSED(Cmd);
    UNUSED(CmdLen);
    
    offset += snprintf(ReplyBuff + offset, ReplyBuffLen - offset, "<ServerFd=%d>{ClientList:", sg_ServerProcWorker.ServerFd);
    ServerClientMapCollectStat(&sg_ServerProcWorker.ClientMap, ReplyBuff, ReplyBuffLen, &offset);
    offset += snprintf(ReplyBuff + offset, ReplyBuffLen - offset, "}");
    
    return MY_SUCCESS;
}

