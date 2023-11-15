#include "include.h"
#include "myModuleCommon.h"
#include "ClientProc.h"

#define MY_CLIENT_ROLE_NAME                                 "QinXClient"
#define MY_CLIENT_CONF_ROOT                                 MY_CLIENT_ROLE_NAME".conf"

void 
ClientExit(
    void
    )
{
    ClientProcExit();
    MyModuleCommonExit();
    system("killall "MY_CLIENT_ROLE_NAME);
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
        else if ((ptr = strstr(line, "ServerIp=")) != NULL)
        {
            len = snprintf(ClientConf->ServerIp, sizeof(ClientConf->ServerIp), "%s", ptr + strlen("ServerIp="));
            if (len <= 0)
            {
                ret = MY_EIO;
                LogErr("Invalid ServerIp %s!", ptr + strlen("ServerIp="));
                goto CommonReturn;
            }
            if ('\n' == ClientConf->ServerIp[len - 1])
            {
                ClientConf->ServerIp[len - 1] = '\0';
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

    ret = ClientProcInit(&clientConfParam);
    if (ret)
    {
        LogErr("ClientProcInit failed! ret %d:%s", ret, My_StrErr(ret));
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
    if (ret)
    {
        ClientProcExit();
        MyModuleCommonExit();
    }
    return ret;
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

    ClientProcMainLoop();
    
CommonReturn:
    if (ret != MY_ERR_EXIT_WITH_SUCCESS)
    {
        ClientExit();
    }
    return ret;
}
