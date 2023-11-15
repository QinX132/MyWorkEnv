#include "include.h"
#include "myModuleCommon.h"
#include "myClientServerMsgs.h"

#define MY_SERVER_MAX_EVENTS                                1024
#define MY_SERVER_COMM_REPLY                                "success"

static pthread_t *sg_ServerProc = NULL;
static BOOL sg_ServerProcShouldExit = FALSE;

static int
_ServerCreateFd(
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

typedef struct _SERVER_EXEC_CMD_ARG
{
    int Fd;
    MY_MSG *Msg;
}
SERVER_EXEC_CMD_ARG;

static void
_ServerMsgExecCmdFn(
    void *Arg
    )
{
    SERVER_EXEC_CMD_ARG *cmdArg = (SERVER_EXEC_CMD_ARG*)Arg;
    int ret = MY_SUCCESS;
    FILE *pipe = NULL;
    char buffer[MY_BUFF_1024] = {0};

    if (!cmdArg || !cmdArg->Msg)
    {
        ret = MY_EIO;
        LogErr("Null input ptr!");
        goto CommonReturn;
    }
    
    pipe = popen((char*)cmdArg->Msg->Cont.VarLenCont, "r");
    if (pipe == NULL) 
    {
        ret = MY_EIO;
        LogErr("popen failed!");
        goto CommonReturn;
    }
    ClearMsgCont(cmdArg->Msg);
    cmdArg->Msg->Head.Type = MY_MSG_TYPE_EXEC_CMD_REPLY;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
    {
        FillMsgCont(cmdArg->Msg, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }

    ret = SendMsg(cmdArg->Fd, cmdArg->Msg);
    if (ret)
    {
        ret = MY_EIO;
        LogErr("Send msg failed!");
        goto CommonReturn;
    }

CommonReturn:
    if (pipe)
        pclose(pipe);
    if (cmdArg)
    {
        if (cmdArg->Msg)
            FreeMsg(cmdArg->Msg);
        MyFree(cmdArg);
    }
    return ;
}

static int
_ServerMsgExecCmd(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;
    SERVER_EXEC_CMD_ARG *arg = NULL;
    MY_MSG *msg = NULL;

    arg = (SERVER_EXEC_CMD_ARG *)MyCalloc(sizeof(SERVER_EXEC_CMD_ARG));
    msg = NewMsg();
    if (likely(arg && msg && Msg))
    {
        memcpy(msg, Msg, sizeof(MY_MSG));
        arg->Msg = msg;
        arg->Fd = Fd;
        ret = TPoolAddTask(_ServerMsgExecCmdFn, arg);
    }
    else
    {
        ret = MY_ENOMEM;
    }

    if (ret)
    {
        if (msg)
            FreeMsg(msg);
        if (arg)
            MyFree(arg);
    }
    return ret;
}

static int
_ServerDefaultMsgHandler(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;
    MY_MSG *replyMsg = NewMsg();
    
    if (!replyMsg)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }
    replyMsg->Head.Type = Msg->Head.Type + 1;

    if (FillMsgCont(replyMsg, Msg->Cont.VarLenCont, Msg->Head.ContentLen - 1) ||
        FillMsgCont(replyMsg, (void*)":", sizeof(char)) ||
        FillMsgCont(replyMsg, (void*)MY_SERVER_COMM_REPLY, sizeof(MY_SERVER_COMM_REPLY))
        )
    {
        ret = MY_EIO;
        LogErr("Fill msg failed");
        goto CommonReturn;
    }
    ret = SendMsg(Fd, replyMsg);
    if (ret)
    {
        LogErr("Send msg failed, ret %d:%s", ret, My_StrErr(ret));
        goto CommonReturn;
    }
CommonReturn:
    if (replyMsg)
    {
        FreeMsg(replyMsg);
    }
    return ret;
}

static
int 
_ServerHandleMsg(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;

    if (!Msg)
    {
        ret = MY_EIO;
        goto CommonReturn;
    }

    switch (Msg->Head.Type)
    {
        case MY_MSG_TYPE_EXEC_CMD:
            ret = _ServerMsgExecCmd(Fd, Msg);
            break;
        default:
            ret = _ServerDefaultMsgHandler(Fd, Msg);
            break;
    }

CommonReturn:
    return ret;
}

static void*
_ServerProcFn(
    void* arg
    )
{
    int serverFd = -1;
    int ret = 0;
    MY_MSG msg;
    UNUSED(arg);

    serverFd = _ServerCreateFd();
    if (0 > serverFd)
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
    event.data.fd = serverFd;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event))
    {
        LogErr("Add to epoll socket failed %d", errno);
        goto CommonReturn;
    }

    int loop = 0;
    /* recv */
    while (!sg_ServerProcShouldExit)
    {
        event_count = epoll_wait(epollFd, waitEvents, MY_SERVER_MAX_EVENTS, 100); 
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
                    struct timeval timeout;
                    timeout.tv_sec = 5;
                    timeout.tv_usec = 0;
                    setsockopt(tmpClientFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

                    event.data.fd = tmpClientFd;
                    event.events = EPOLLIN | EPOLLET;  
                    epoll_ctl(epollFd, EPOLL_CTL_ADD, tmpClientFd, &event);
                    LogInfo("Recv new connection, fd %d", tmpClientFd);
                }
            }
            else if (waitEvents[loop].events & EPOLLIN)
            {
                ret = RecvMsg(waitEvents[loop].data.fd, &msg);
                if (ret == MY_SUCCESS)
                {
                    ret = _ServerHandleMsg(waitEvents[loop].data.fd, &msg);
                    if (ret)
                    {
                        LogDbg("Handle msg filed %d", ret);
                    }
                }
                else
                {
                    if (MY_ERR_PEER_CLOSED == ret)
                    {
                        LogDbg("Peer closed , fd = %d", waitEvents[loop].data.fd);
                    }
                    else
                    {
                        LogErr("Recv from %d failed %d:%s", waitEvents[loop].data.fd, ret, My_StrErr(ret));
                    }
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                    close(waitEvents[loop].data.fd);
                }
            }
            else if (waitEvents[loop].events & EPOLLERR || waitEvents[loop].events & EPOLLHUP)
            {
                LogInfo("%d error happen!", waitEvents[loop].data.fd);
                epoll_ctl(epollFd, EPOLL_CTL_DEL, waitEvents[loop].data.fd, NULL);
                close(waitEvents[loop].data.fd);
            }
        }
    }

CommonReturn:
    if (serverFd != -1)
        close(serverFd);
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
    
    if (!sg_ServerProc)
    {
        sg_ServerProc = (pthread_t*)MyCalloc(sizeof(pthread_t));
        if (!sg_ServerProc)
        {
            ret = MY_ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        sg_ServerProcShouldExit = FALSE;
        ret = pthread_create(sg_ServerProc, NULL, _ServerProcFn, NULL);
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
    if (sg_ServerProc)
    {
        sg_ServerProcShouldExit = TRUE;
        pthread_join(*sg_ServerProc, NULL);
        MyFree(sg_ServerProc);
        sg_ServerProc = NULL;
    }
}
