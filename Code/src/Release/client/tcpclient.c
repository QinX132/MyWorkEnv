#include "include.h"
#include "myMsg.h"
#include "myLogIO.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"
#include "myModuleCommon.h"

#define MY_CLIENT_ROLE_NAME                                 "tcpclient"
#define MY_CLIENT_CONF_ROOT                                 MY_CLIENT_ROLE_NAME".conf"

typedef struct _CLIENT_CONF_PARAM
{
    char PeerIp[MY_BUFF_64];
    MY_LOG_LEVEL LogLevel;
    char LogFilePath[MY_BUFF_64];
}
CLIENT_CONF_PARAM;

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

static int
_ClientCreateFd(
    char *Ip,
    uint16_t Port,
    __out int32_t *Fd
    )
{
    int clientFd = -1;
    unsigned int serverIp = 0;
    int ret = 0;
    struct sockaddr_in serverAddr = {0};
    int32_t reuseable = 1; // set port reuseable when fd closed
    struct timeval timeout;

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > clientFd)
    {
        ret = errno;
        LogErr("Create socket failed");
        goto CommonReturn;
    }
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        ret = errno;
        LogErr("setsockopt failed %d %s", errno, My_StrErr(errno));
        goto CommonReturn;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Port);
    inet_pton(AF_INET, Ip, &serverIp);
    serverAddr.sin_addr.s_addr=serverIp;
    if(0 > connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
    {
        ret = errno;
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
    UNUSED(Event);
    UNUSED(Arg);
    
    MY_MSG msg;
    (void)RecvMsg(Fd, &msg);
    printf("<<<< RecvMsg: %s\n", (char*)msg.Cont.VarLenCont);
}

#define MY_CLIENT_WORKER_KEEPALIVE_INTERVAL                             1 // s
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
_ClientWorkerProc(
    void* Arg
    )
{
    int ret = 0;
    CLIENT_CONF_PARAM *confArg = (CLIENT_CONF_PARAM*)Arg;
    struct timeval tv;
    
    ret = _ClientCreateFd(confArg->PeerIp, MY_TCP_SERVER_PORT, &sg_ClientWorker.ClientFd);
    if (ret)
    {
        LogErr("Create fd failed, %d:%s!", ret, My_StrErr(ret));
        goto CommonReturn;
    }

    sg_ClientWorker.EventBase = event_base_new();
    if (!sg_ClientWorker.EventBase)
    {
        ret = MY_ENOMEM;
        LogErr("event_base_new failed!");
        goto CommonReturn;
    }
    if (evthread_make_base_notifiable(sg_ClientWorker.EventBase) < 0)
    {
        ret = MY_EIO;
        goto CommonReturn;
    }
    sg_ClientWorker.RecvEvent = event_new(sg_ClientWorker.EventBase, sg_ClientWorker.ClientFd, 
                                            EV_READ | EV_PERSIST, _ClientRecvMsg, NULL);
    sg_ClientWorker.Keepalive= event_new(sg_ClientWorker.EventBase, -1, 
                                            EV_READ | EV_PERSIST, _ClientKeepalive, NULL);
    if (!sg_ClientWorker.RecvEvent || !sg_ClientWorker.Keepalive)
    {
        ret = MY_ENOMEM;
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

static int 
_ClientParseConf(
    CLIENT_CONF_PARAM *ClientConf
    )
{
    int ret = 0;
    FILE *fp = NULL;
    char line[MY_BUFF_128] = {0};
    char *ptr = NULL;
    int len = 0;

    fp = fopen(MY_CLIENT_CONF_ROOT, "r");
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
        else if ((ptr = strstr(line, "PeerIp=")) != NULL)
        {
            len = snprintf(ClientConf->PeerIp, sizeof(ClientConf->PeerIp), "%s", ptr + strlen("PeerIp="));
            if (len <= 0)
            {
                ret = MY_EIO;
                LogErr("Invalid PeerIp %s!", ptr + strlen("PeerIp="));
                goto CommonReturn;
            }
            if ('\n' == ClientConf->PeerIp[len - 1])
            {
                ClientConf->PeerIp[len - 1] = '\0';
            }
        }
        else if ((ptr = strstr(line, "LogLevel=")) != NULL)
        {
            ClientConf->LogLevel = atoi(ptr + strlen("LogLevel="));
            if (!(ClientConf->LogLevel >= MY_LOG_LEVEL_INFO && ClientConf->LogLevel <= MY_LOG_LEVEL_ERROR))
            {
                ret = MY_EIO;
                LogErr("Invalid loglevel!");
                goto CommonReturn;
            }
        }
        else if ((ptr = strstr(line, "LogPath=")) != NULL)
        {
            len = snprintf(ClientConf->LogFilePath, sizeof(ClientConf->LogFilePath), "%s", ptr + strlen("LogPath="));
            if (len <= 0)
            {
                ret = MY_EIO;
                LogErr("Invalid LogPath %s!", ptr + strlen("LogPath="));
                goto CommonReturn;
            }
            if ('\n' == ClientConf->LogFilePath[len - 1])
            {
                ClientConf->LogFilePath[len - 1] = '\0';
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

static int 
_ClientWorkerInit(
    CLIENT_CONF_PARAM *ClientConfParam
    )
{
    int ret = MY_SUCCESS;
    int sleepIntervalUs = 10;
    int waitTimeUs = sleepIntervalUs * 1000; // 10 ms
    
    if (!ClientConfParam)
    {
        ret = MY_EINVAL;
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
        ret = MY_ENOMEM;
        LogErr("Apply memory failed!");
        goto CommonReturn;
    }
    ret = pthread_create(sg_ClientWorker.Thread, NULL, _ClientWorkerProc, (void*)ClientConfParam);
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

static void
_ClientWorkerExit(
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
_ClientInit(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    MY_MODULES_INIT_PARAM initParam;
    CLIENT_CONF_PARAM clientConfParam;
    
    UNUSED(argc);
    UNUSED(argv);
    memset(&initParam, 0, sizeof(initParam));
    memset(&clientConfParam, 0, sizeof(clientConfParam));

    ret = _ClientParseConf(&clientConfParam);
    if (ret)
    {
        LogErr("Init conf failed!");
        goto CommonReturn;
    }

    initParam.HealthArg = NULL;
    initParam.InitTimerModule = FALSE;
    initParam.CmdLineArg = NULL;
    initParam.TPoolArg = NULL;
    initParam.InitMsgModule = TRUE;
    initParam.LogArg = (MY_LOG_MODULE_INIT_ARG*)calloc(sizeof(MY_LOG_MODULE_INIT_ARG), 1);
    if (!initParam.LogArg)
    {
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    // log init args
    initParam.LogArg->LogFilePath = clientConfParam.LogFilePath;
    initParam.LogArg->LogLevel = clientConfParam.LogLevel;
    initParam.LogArg->RoleName = MY_CLIENT_ROLE_NAME;
    ret = MyModuleCommonInit(initParam);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("MyModuleCommonInit failed!");
        }
        goto CommonReturn;
    }

    ret = _ClientWorkerInit(&clientConfParam);
    if (ret)
    {
        LogErr("_ClientWorkerInit failed! ret %d:%s", ret, My_StrErr(ret));
        goto CommonReturn;
    }
    
CommonReturn:
    if (initParam.LogArg)
    {
        free(initParam.LogArg);
    }
    if (initParam.TPoolArg)
    {
        free(initParam.TPoolArg);
    }
    return ret;
}

void 
ClientExit(
    void
    )
{
    _ClientWorkerExit();
    MyModuleCommonExit();
}

void
_ClientMainLoop(
    void
    )
{
    int ret = MY_SUCCESS;
    MY_MSG *msgToSend = NULL;
    size_t bufLen = MY_MSG_CONTENT_MAX_LEN;
    char *buf = NULL;
    uint32_t inputLen = 0;
    struct timeval tv;
    uint32_t currentSessionId = 0;

    buf = MyCalloc(bufLen);
    if (!buf)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }
    
    while (sg_ClientWorker.IsRunning)
    {
        memset(buf, 0, bufLen);
        fgets(buf, bufLen, stdin);
        *(strchr(buf, '\n')) = '\0';
        inputLen = strnlen(buf, MY_MSG_CONTENT_MAX_LEN - 1) + 1;
        if (strcasecmp(buf, MY_DISCONNECT_STRING) == 0)
        {
            break;
        }

        msgToSend = NewMsg();
        if (!msgToSend)
        {
            goto CommonReturn;
        }

        gettimeofday(&tv, NULL);
        msgToSend->Head.MsgContentLen = inputLen;
        msgToSend->Head.MagicVer = 0xff;
        msgToSend->Head.SessionId = currentSessionId ++;
        memcpy(msgToSend->Cont.VarLenCont, buf, msgToSend->Head.MsgContentLen);
        msgToSend->Tail.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;

        ret = SendMsg(sg_ClientWorker.ClientFd, msgToSend);
        if (ret)
        {
            LogErr("Send msg failed!");
            goto CommonReturn;
        }
        
        FreeMsg(msgToSend);
        msgToSend = NULL;
    }
    
CommonReturn:
    if (msgToSend)
        FreeMsg(msgToSend);
    if (buf)
        MyFree(buf);
    return ;
}

int main(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    
    ret = _ClientInit(argc, argv);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            printf("Client init failed! ret %d", ret);
        }
        goto CommonReturn;
    }

    _ClientMainLoop();
    
CommonReturn:
    if (ret != MY_ERR_EXIT_WITH_SUCCESS)
    {
        ClientExit();
    }
    return ret;
}
