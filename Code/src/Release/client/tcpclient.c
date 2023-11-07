#include "include.h"
#include "myMsg.h"
#include "myLogIO.h"
#include "myThreadPool.h"
#include "myModuleHealth.h"

static uint32_t currentSessionId = 0;
#define MY_CLIENT_ROLE_NAME                                 "tcpclient"
#define MY_CLIENT_CONF_ROOT                                 MY_CLIENT_ROLE_NAME".conf"

pthread_t *ClientMsgHandler = NULL;

typedef struct {
    char PeerIp[MY_BUFF_64];
    MY_LOG_LEVEL LogLevel;
    char LogFilePath[MY_BUFF_64];
}
CLIENT_CONF_PARAM;

static int
_Client_CreateFd(
    char *Ip
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
    serverAddr.sin_port = htons(MY_TCP_SERVER_PORT);
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

    MY_MSG *msgToSend = NULL;
    /* send */
    while (1)
    {
        char buf[4096] = {0};
        printf("Send:");
        scanf("%s", buf);
        uint32_t stringLen = strlen(buf) + 1;
        if (strcasecmp(buf, MY_DISCONNECT_STRING) == 0)
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
        
        MY_MSG msg;
        (void)RecvMsg(clientFd, &msg);
        printf("\t Recv:%s\n", (char*)msg.Cont.VarLenCont);

        FreeMsg(msgToSend);
        msgToSend = NULL;
    }

CommonReturn:
    if (clientFd > 0)
        close(clientFd);
    if (msgToSend)
        FreeMsg(msgToSend);
    return NULL;
}

static int 
_Client_ParseConf(
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
_Client_Init(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    MY_MODULES_INIT_PARAM initParam;
    static CLIENT_CONF_PARAM clientConfParam;
    
    UNUSED(argc);
    UNUSED(argv);
    memset(&initParam, 0, sizeof(initParam));
    memset(&clientConfParam, 0, sizeof(clientConfParam));

    ret = _Client_ParseConf(&clientConfParam);
    if (ret)
    {
        LogErr("Init conf failed!");
        goto CommonReturn;
    }

    initParam.InitHealthModule = TRUE;
    initParam.InitMsgModule = TRUE;
    initParam.InitTimerModule = TRUE;
    initParam.CmdLineArg = NULL;
    initParam.LogArg = (MY_LOG_MODULE_INIT_ARG*)calloc(sizeof(MY_LOG_MODULE_INIT_ARG), 1);
    initParam.TPoolArg = (MY_TPOOL_MODULE_INIT_ARG*)calloc(sizeof(MY_TPOOL_MODULE_INIT_ARG), 1);
    if (!initParam.LogArg || !initParam.TPoolArg)
    {
        LogErr("Apply mem failed!");
        goto CommonReturn;
    }
    // log init args
    initParam.LogArg->LogFilePath = clientConfParam.LogFilePath;
    initParam.LogArg->LogLevel = clientConfParam.LogLevel;
    initParam.LogArg->RoleName = MY_CLIENT_ROLE_NAME;
    // tpool init args
    initParam.TPoolArg->ThreadPoolSize = 5;
    initParam.TPoolArg->Timeout = 5;
    initParam.TPoolArg->TaskListMaxLength = 1024;
    ret = MyModuleCommonInit(initParam);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("MyModuleCommonInit failed!");
        }
        goto CommonReturn;
    }
    
    if (!ClientMsgHandler)
    {
        ClientMsgHandler = (pthread_t*)MyCalloc(sizeof(pthread_t));
        if (!ClientMsgHandler)
        {
            ret = ENOMEM;
            LogErr("Apply memory failed!");
            goto CommonReturn;
        }
        ret = pthread_create(ClientMsgHandler, NULL, _Client_WorkerFunc, (void*)clientConfParam.PeerIp);
        if (ret) 
        {
            LogErr("Failed to create thread");
            goto CommonReturn;
        }
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
Client_Exit(
    void
    )
{
    // msg handler
    LogInfo("----------------- MsgHandler exiting!-------------------");
    if (ClientMsgHandler)
    {
        MyFree(ClientMsgHandler);
        ClientMsgHandler = NULL;
    }
    LogInfo("----------------- MsgHandler exited! -------------------");
    MyModuleCommonExit();
}

int main(
    int argc,
    char *argv[]
    )
{
    int ret = 0;
    
    ret = _Client_Init(argc, argv);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            LogErr("Client init failed! ret %d", ret);
        }
        goto CommonReturn;
    }
    
    LogInfo("Joining thread: Client Worker Func");
    pthread_join(*ClientMsgHandler, NULL);

CommonReturn:
    if (ret != MY_ERR_EXIT_WITH_SUCCESS)
    {
        Client_Exit();
    }
    return ret;
}
