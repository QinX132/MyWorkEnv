#include "myLogIO.h"
#include "myModuleHealth.h"

#define MY_TEST_LOG_MEX_LEN             (50 * 1024 * 1024)

static char sg_RoleName[MY_TEST_ROLE_NAME_MAX_LEN] = {0};
static char *sg_LogLevelStr [MY_TEST_LOG_LEVEL_MAX] = 
{
    [MY_TEST_LOG_LEVEL_INFO] = "INFO",
    [MY_TEST_LOG_LEVEL_DEBUG] = "DEBUG",
    [MY_TEST_LOG_LEVEL_WARNING] = "WARN",
    [MY_TEST_LOG_LEVEL_ERROR] = "ERROR",
};
static pthread_spinlock_t sg_LogSpinlock;
static char sg_LogPath[MY_TEST_BUFF_128] = {0};
static MY_TEST_LOG_LEVEL sg_LogLevel = MY_TEST_LOG_LEVEL_INFO;
static BOOL sg_LogModuleInited = FALSE;
static FILE* sg_LogFileFp = NULL;

static int sg_LogPrinted = 0;

int
LogModuleInit(
    MY_LOG_MODULE_INIT_ARG *InitArg
    )
{
    int ret = 0;

    if (!InitArg|| !InitArg->LogFilePath || !InitArg->RoleName)
    {
        ret = MY_EINVAL;
        LogErr("Too long role name!");
        goto CommonReturn;
    }
    strcpy(sg_RoleName, InitArg->RoleName);
    strcpy(sg_LogPath, InitArg->LogFilePath);
    sg_LogLevel = InitArg->LogLevel;
    pthread_spin_init(&sg_LogSpinlock, PTHREAD_PROCESS_PRIVATE);

    sg_LogFileFp = fopen(sg_LogPath, "a");
    if (!sg_LogFileFp)
    {
        ret = MY_EIO;
        LogErr("Open file failed!");
        goto CommonReturn;
    }
    sg_LogModuleInited = TRUE;
    
CommonReturn:
    return ret;
}

void
LogPrint(
    int level,
    const char* Function,
    int Line,
    const char* Fmt,
    ...
    )
{
    va_list args;

    if (level < (int)sg_LogLevel)
    {
        return;
    }

    if (!sg_LogModuleInited)
    {
        va_start(args, Fmt);
        vprintf(Fmt, args);
        va_end(args);
        printf("\n");
        return;
    }
    
    pthread_spin_lock(&sg_LogSpinlock);
    
    va_start(args, Fmt);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);
    char timestamp[24] = {0};
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d_%H:%M:%S", tm_info);
    int milliseconds = tv.tv_usec / 1000;
    fprintf(sg_LogFileFp, "[%s.%03d][%s][%s-%d]%s:", timestamp, milliseconds, sg_LogLevelStr[level], Function, Line, sg_RoleName);
    vfprintf(sg_LogFileFp, Fmt, args);
    va_end(args);
    fprintf(sg_LogFileFp, "\n");
    fflush(sg_LogFileFp);

    sg_LogPrinted ++;
    
    pthread_spin_unlock(&sg_LogSpinlock);
}

void
LogModuleExit(
    void
    )
{
    if (sg_LogModuleInited)
    {
        pthread_spin_lock(&sg_LogSpinlock);
        pthread_spin_unlock(&sg_LogSpinlock);
        pthread_spin_destroy(&sg_LogSpinlock);
        // fclose(sg_LogFileFp);  //we do not close fp
    }
}

int
LogModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = 0;
    FILE* fp = fopen(sg_LogPath, "rb");
    long long size = 0;
    int len = 0;
    
    if (!fp) 
    {
        LogErr("Failed to open file: %s\n", sg_LogPath);
        goto CommonReturn;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
        "<%s:[LogPrinted:%d, LogSize:%lld]>", ModuleNameByEnum(MY_MODULES_ENUM_LOG), sg_LogPrinted, size);
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }
    
CommonReturn:
    if (fp)
        fclose(fp);

    if (size >= MY_TEST_LOG_MEX_LEN)
    {
        pthread_spin_lock(&sg_LogSpinlock);
        fprintf(sg_LogFileFp, "Lograting!\n");
        fclose(sg_LogFileFp);
        char cmd[128] = {0};
        if (strlen(sg_LogPath))
        {
            sprintf(cmd, "mv %s %s.0", sg_LogPath, sg_LogPath);
        }
        system(cmd);
        sg_LogFileFp = fopen(sg_LogPath, "a");
        pthread_spin_unlock(&sg_LogSpinlock);
    }
    return ret;
}

void
SetLogLevel(
    uint32_t LogLevel
    )
{
    if (LogLevel <= MY_TEST_LOG_LEVEL_ERROR)
    {
        sg_LogLevel = LogLevel;
    }
}
