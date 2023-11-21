#include "include.h"
#include "myClientServerMsgs.h"
#include "ClientProc.h"
#include "ClientMsgHandler.h"

#define MY_CLIENT_WORKER_KEEPALIVE_INTERVAL                             1 // s

typedef struct _CLIENT_WORKER
{
    int ClientFd;
    pthread_t *Thread;
    struct event_base *EventBase;
    struct event *Keepalive;
    struct event *RecvEvent;
    BOOL IsRunning;
}
CLIENT_WORKER;

static CLIENT_WORKER sg_ClientWorker = {
        .ClientFd = -1,
        .Thread = 0,
        .EventBase = NULL,
        .RecvEvent = NULL,
        .IsRunning = FALSE
    };

extern void 
ClientExit(
    void
    );

void
ClientProcExit(
    void
    )
{
    if (sg_ClientWorker.EventBase && sg_ClientWorker.Thread)
    {
        event_base_loopexit(sg_ClientWorker.EventBase, NULL);
        pthread_join(*sg_ClientWorker.Thread, NULL);
        sg_ClientWorker.Thread = NULL;
    }
}

static int
_ClientCreateFd(
    char *Ip,
    uint16_t Port,
    __out int32_t *Fd
    )
{
    int clientFd = -1;
    unsigned int serverIp = 0;
    int ret = MY_SUCCESS;
    struct sockaddr_in serverAddr = {0};
    struct sockaddr_in localAddr = {0};
    int32_t reuseable = 1; // set port reuseable when fd closed
    struct timeval timeout;

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > clientFd)
    {
        ret = -errno;
        LogErr("Create socket failed");
        goto CommonReturn;
    }
    LogInfo("Open clientFd %d", clientFd);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(0);
    localAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(clientFd, (void *)&localAddr, sizeof(localAddr)))
    {
        ret = -errno;
        LogErr("Bind failed");
        goto CommonReturn;
    }
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));
    
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        ret = -errno;
        LogErr("setsockopt failed %d %s", errno, My_StrErr(errno));
        goto CommonReturn;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Port);
    inet_pton(AF_INET, Ip, &serverIp);
    serverAddr.sin_addr.s_addr = serverIp;
    if(connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
    {
        ret = -errno;
        LogErr("Connect failed");
        goto CommonReturn;
    }
    LogInfo("Connect success");

CommonReturn:
    if (ret)
    {
        if (clientFd != -1)
            close(clientFd);
        clientFd = -1;
    }
    else
    {
        *Fd = clientFd;
    }
    return ret;
}

void
_ClientRecvMsg(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    int ret = MY_SUCCESS;
    MY_MSG msg;
    UNUSED(Event);
    UNUSED(Arg);
    
    ret = RecvMsg(Fd, &msg);
    if (ret == MY_SUCCESS)
    {
        ret = ClientHandleMsg(Fd, &msg);
        if (ret)
        {
            LogErr("handle msg failed, ret %d:%s\n", ret, My_StrErr(ret));
        }
    }
    else
    {
        printf("Error happen, ret %d:%s\n", ret, My_StrErr(ret));
        ClientExit();
    }
}

void
_ClientKeepalive(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    UNUSED(Fd);
    UNUSED(Event);
    UNUSED(Arg);
    return ;
}

void*
_ClientProcFn(
    void* Arg
    )
{
    int ret = MY_SUCCESS;
    CLIENT_CONF_PARAM *confArg = (CLIENT_CONF_PARAM*)Arg;
    struct timeval tv;
    
    ret = _ClientCreateFd(confArg->ServerIp, MY_TCP_SERVER_PORT, &sg_ClientWorker.ClientFd);
    if (ret)
    {
        LogErr("Create fd failed, %d:%s!", ret, My_StrErr(ret));
        goto CommonReturn;
    }

    sg_ClientWorker.EventBase = event_base_new();
    if (!sg_ClientWorker.EventBase)
    {
        ret = -MY_ENOMEM;
        LogErr("event_base_new failed!");
        goto CommonReturn;
    }
    if (evthread_make_base_notifiable(sg_ClientWorker.EventBase) < 0)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }
    sg_ClientWorker.RecvEvent = event_new(sg_ClientWorker.EventBase, sg_ClientWorker.ClientFd, 
                                            EV_READ | EV_PERSIST, _ClientRecvMsg, NULL);
    sg_ClientWorker.Keepalive= event_new(sg_ClientWorker.EventBase, -1, 
                                            EV_READ | EV_PERSIST, _ClientKeepalive, NULL);
    if (!sg_ClientWorker.RecvEvent || !sg_ClientWorker.Keepalive)
    {
        ret = -MY_ENOMEM;
        LogErr("event_new failed!");
        goto CommonReturn;
    }
    event_add(sg_ClientWorker.RecvEvent, NULL);
    tv.tv_sec = MY_CLIENT_WORKER_KEEPALIVE_INTERVAL;
    tv.tv_usec = 0;
    event_add(sg_ClientWorker.Keepalive, &tv);
    event_active(sg_ClientWorker.Keepalive, EV_READ, 0);
    sg_ClientWorker.IsRunning = TRUE;
    
    event_base_dispatch(sg_ClientWorker.EventBase);

CommonReturn:
    sg_ClientWorker.IsRunning = FALSE;
    if (sg_ClientWorker.ClientFd > 0)
    {
        close(sg_ClientWorker.ClientFd);
        sg_ClientWorker.ClientFd = -1;
    }
    if (sg_ClientWorker.Keepalive)
    {
        event_free(sg_ClientWorker.Keepalive);
        sg_ClientWorker.Keepalive = NULL;
    }
    if (sg_ClientWorker.RecvEvent)
    {
        event_free(sg_ClientWorker.RecvEvent);
        sg_ClientWorker.RecvEvent = NULL;
    }
    if (sg_ClientWorker.EventBase)
    {
        event_base_free(sg_ClientWorker.EventBase);
        sg_ClientWorker.EventBase = NULL;
    }
    return NULL;
}

void
ClientProcMainLoop(
    void
    )
{
    int ret = MY_SUCCESS;
    MY_MSG *msgToSend = NULL;
    size_t bufLen = MY_MSG_CONTENT_MAX_LEN;
    char *buf = NULL;
    int32_t msgType = MY_MSG_TYPE_START_UNSPEC;
    uint32_t inputLen = 0;
    char msgContbuff[MY_BUFF_1024] = {0};

    buf = MyCalloc(bufLen);
    if (!buf)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    
    while (sg_ClientWorker.IsRunning)
    {
        memset(buf, 0, bufLen);
        fgets(buf, bufLen, stdin);
        *(strchr(buf, '\n')) = '\0';
        inputLen = strnlen(buf, MY_MSG_CONTENT_MAX_LEN - 1) + 1;

        if (ClientCmdIsExit(buf))
        {
            printf("%s is exiting now...\n", MY_CLIENT_ROLE_NAME);
            fflush(stdout);
            break;
        }

        memset(msgContbuff, 0, sizeof(msgContbuff));
        if (!ClientCmdSupported(buf, inputLen, &msgType, msgContbuff, sizeof(msgContbuff)))
        {
            ClientCmdUsage();
            fflush(stdout);
            continue;
        }

        ret = ClientHandleCmd(sg_ClientWorker.ClientFd, msgType, 
                        msgContbuff, strnlen(msgContbuff, sizeof(msgContbuff)));
        if (ret)
        {
            LogErr("Cmd handle failed! msgType%d, msgContbuff:%s, ret %d:%s", 
                msgType, msgContbuff, ret, My_StrErr(ret));
        }
    }
    
CommonReturn:
    if (msgToSend)
        FreeMsg(msgToSend);
    if (buf)
        MyFree(buf);
    return ;
}

int
ClientProcInit(
    CLIENT_CONF_PARAM *ClientConfParam
    )
{
    int ret = MY_SUCCESS;
    int sleepIntervalUs = 10;
    int waitTimeUs = sleepIntervalUs * 1000; // 10 ms
    
    if (!ClientConfParam)
    {
        ret = -MY_EINVAL;
        goto CommonReturn;
    }

    if (sg_ClientWorker.ClientFd != -1 && sg_ClientWorker.Thread && 
        sg_ClientWorker.EventBase && sg_ClientWorker.RecvEvent)
    {
        goto CommonReturn;
    }

    sg_ClientWorker.Thread = (pthread_t*)MyCalloc(sizeof(pthread_t));
    if (!sg_ClientWorker.Thread)
    {
        ret = -MY_ENOMEM;
        LogErr("Apply memory failed!");
        goto CommonReturn;
    }
    ret = pthread_create(sg_ClientWorker.Thread, NULL, _ClientProcFn, (void*)ClientConfParam);
    if (ret) 
    {
        LogErr("Failed to create thread");
        goto CommonReturn;
    }

    while(!sg_ClientWorker.IsRunning && waitTimeUs >= 0)
    {
        usleep(sleepIntervalUs);
        waitTimeUs -= sleepIntervalUs;
    }
    
CommonReturn:
    return ret;
}

