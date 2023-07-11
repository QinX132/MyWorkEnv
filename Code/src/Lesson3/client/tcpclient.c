#include "include.h"
#include "myMsg.h"
#include "myLogIO.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"

static uint32_t currentSessionId = 0;
#define MY_TEST_CLIENT_ROLE_NAME                                "TcpClient"
#define MY_TEST_CLIENT_TID_FILE                                 "TcpClient.tid"

pthread_t *ClientMsgHandler = NULL;
MY_TEST_THREAD_POOL *ClientThreadPool = NULL;

static int
_Client_CreateFd(
    char *Ip
    )
{
    int clientFd = -1;
    unsigned int serverIp = 0;
    struct sockaddr_in serverAddr = {0};
    int32_t reuseable = 1; // set port reuseable when fd closed

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if(0 > clientFd)
    {
        LogErr("Create socket failed");
        goto CommonReturn;
    }
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(MY_TEST_TCP_SERVER_PORT);
    inet_pton(AF_INET, Ip, &serverIp);
    serverAddr.sin_addr.s_addr=serverIp;
    if(0 > connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
    {
        LogErr("Connect failed");
        goto CommonReturn;
    }
    LogInfo("Connect success");

CommonReturn:
    return clientFd;
}

void*
_Client_WorkerFunc(
    void* arg
    )
{
    struct timeval tv;
    int clientFd = -1;
    int ret = 0;
    
    clientFd = _Client_CreateFd((char*)arg);
    if (clientFd == -1)
    {
        LogErr("Create fd failed!");
        goto CommonReturn;
    }

    MY_TEST_MSG *msgToSend = NULL;
    /* send */
    while (1)
    {
	    char buf[4096] = {0};
        scanf("%s",buf);
        uint32_t stringLen = strlen(buf) + 1;
        if (strcasecmp(buf, MY_TEST_DISCONNECT_STRING) == 0)
        {
            break;
        }

        msgToSend = NewMsg();
        if (!msgToSend)
        {
            goto CommonReturn;
        }

        msgToSend->Head.MsgId = 1;
        gettimeofday(&tv, NULL);
        msgToSend->Head.MsgContentLen = stringLen;
        msgToSend->Head.MagicVer = 0xff;
        msgToSend->Head.SessionId = currentSessionId ++;
        memcpy(msgToSend->Cont.VarLenCont, buf, stringLen);
        msgToSend->Tail.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;

        ret = SendMsg(clientFd, *msgToSend);
        if (ret)
        {
            LogErr("Send msg failed!");
            goto CommonReturn;
        }

        (void)RecvMsg(clientFd);

        FreeMsg(msgToSend);
        msgToSend = NULL;
    }

CommonReturn:
    if (clientFd != -1)
        close(clientFd);
    if (msgToSend)
        free(msgToSend);
    return NULL;
}

static int
_Client_Init(
    char *PeerIp
    )
{
    int ret = 0;
    pthread_attr_t attr;
    BOOL attrInited = FALSE;

    ret = LogModuleInit(MY_TEST_LOG_FILE, MY_TEST_CLIENT_ROLE_NAME, strlen(MY_TEST_CLIENT_ROLE_NAME));
    if (ret)
    {
        printf("Init log failed!");
        goto CommonReturn;
    }
    MsgModuleInit();
    
    if (!ClientMsgHandler)
    {
        ClientMsgHandler = (pthread_t*)malloc(sizeof(pthread_t));
        if (!ClientMsgHandler)
        {
            ret = ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        pthread_attr_init(&attr);
        attrInited = TRUE;
        ret = pthread_create(ClientMsgHandler, &attr, _Client_WorkerFunc, (void*)PeerIp);
        if (ret) 
        {
            LogErr("Failed to create thread");
            goto CommonReturn;
        }
    }
    if (!ClientThreadPool)
    {
        ClientThreadPool = (MY_TEST_THREAD_POOL*)malloc(sizeof(MY_TEST_THREAD_POOL));
        if (!ClientThreadPool)
        {
            ret = ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        (void)ThreadPoolModuleInit(ClientThreadPool, 10, 5);
    }
    
    ret = HealthModuleInit();
    if (ret)
    {
        LogErr("HealthModuleInit failed!");
    }
    
CommonReturn:
    if (attrInited)
        pthread_attr_destroy(&attr);
    return ret;
}

static void 
_Client_Exit(
    void
    )
{
    // health
    LogInfo("----------------- HealthModule exiting!-------------------");
    HealthModuleExit();
    LogInfo("----------------- HealthModule exited! -------------------");
    // msg handler
    LogInfo("----------------- MsgHandler exiting!-------------------");
    if (ClientMsgHandler)
    {
        free(ClientMsgHandler);
        ClientMsgHandler = NULL;
    }
    LogInfo("----------------- MsgHandler exited! -------------------");
    // TPool
    LogInfo("----------------- TPoolModule exiting!------------------");
    if (ClientThreadPool)
    {
        ThreadPoolModuleExit(ClientThreadPool);
        free(ClientThreadPool);
        ClientThreadPool = NULL;
    }
    LogInfo("----------------- TPoolModule exited! ------------------");
    // msg
    LogInfo("----------------- MsgModule exiting! -------------------");
    MsgModuleExit();
    LogInfo("----------------- MsgModule exited! --------------------");
    // log
    LogInfo("----------------- LogModule exiting! -------------------");
    LogModuleExit();
}

int main(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    
    if (argc != 2)
    {
        printf("Usage: ./tcpclient ip\n");
        goto CommonReturn;
    }
    
    ret = _Client_Init(argv[1]);
    if (ret)
    {
        LogErr("Client init failed! ret %d", ret);
        goto CommonReturn;
    }
    
    LogInfo("Joining thread: Client Worker Func");
    pthread_join(*ClientMsgHandler, NULL);

CommonReturn:
    _Client_Exit();
    return ret;
}
