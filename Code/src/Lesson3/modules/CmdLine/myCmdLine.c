#include "myLogIO.h"

static MY_TEST_CMDLINE_ROLE sg_CmdLineRole = MY_TEST_CMDLINE_ROLE_UNUSED;
static pthread_t *sg_CmdLineWorker = NULL;

void
_CmdServer_WorkerFunc(
    void* arg
    )
{
    int ret = 0;
    int serverFd = *(int*)arg;
    int clientFd = -1;
    struct sockaddr clientaddr;
    socklen_t clientLen;

    if(0 > listen(serverFd, 1))
    {
        ret = -1;
        LogErr("Listen failed");
        goto CommonReturn;
    }
    
    clientFd = accept(serverFd, &clientaddr, &clientLen);
    if (clientFd == -1)
    {
        ret = errno;
        LogErr("accept failed %d %s", ret, strerrno(ret));
        goto CommonReturn;
    }

    while(1)
    
CommonReturn:
    return;
}

int
CmdLineModuleInit(
    MY_TEST_CMDLINE_ROLE Role
    )
{
    int ret = 0;
    pthread_attr_t attr;
    BOOL attrInited = FALSE;

    sg_CmdLineRole = Role;

    switch (sg_CmdLineRole)
    {
        case MY_TEST_CMDLINE_ROLE_SVR:
            int serverFd = -1;
            int32_t reuseable = 1; // set port reuseable when fd closed
            struct sockaddr_in localAddr = {0};
            
            serverFd = socket(AF_INET, SOCK_STREAM, 0);
            (void)setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));    // set reuseable

            localAddr.sin_family = AF_INET;
            localAddr.sin_port = htons(MY_TEST_SERVER_CMD_LINE_PORT);
            localAddr.sin_addr.s_addr=htonl(INADDR_ANY);

            if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
            {
                ret = -1;
                LogErr("Bind failed");
                goto CommonReturn;
            }
            LogInfo("Bind cmdserver serverFd: %d", serverFd);

            sg_CmdLineWorker = (pthread_t*)malloc(sizeof(pthread_t));
            if (!sg_CmdLineWorker)
            {
                ret = ENOMEM;
                LogErr("Apply memory failed!");
                goto CommonReturn;
            }
            pthread_attr_init(&attr);
            attrInited = TRUE;
            ret = pthread_create(sg_CmdLineWorker, &attr, _CmdServer_WorkerFunc, &serverFd);
            if (ret) 
            {
                LogErr("Failed to create thread");
                goto CommonReturn;
            }
        case MY_TEST_CMDLINE_ROLE_CLT:
            
        default:
            ret = EINVAL;
            LogErr("Role %d invalid!", sg_CmdLineRole);
            break;
    }
    
CommonReturn:
    if (attrInited)
        pthread_attr_destroy(&attr);
    return ret;
}

