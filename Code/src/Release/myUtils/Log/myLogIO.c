#include "myLogIO.h"
#include "myModuleHealth.h"

#define MY_LOG_MEX_LEN                                  (50 * 1024 * 1024)

static char *sg_LogLevelStr [MY_LOG_LEVEL_MAX] = 
{
    [MY_LOG_LEVEL_INFO] = "INFO",
    [MY_LOG_LEVEL_DEBUG] = "DEBUG",
    [MY_LOG_LEVEL_WARNING] = "WARN",
    [MY_LOG_LEVEL_ERROR] = "ERROR",
};

typedef struct _MY_LOG_WORKER_STATS
{
    int LogPrinted;
    long long LogSize;
}
MY_LOG_WORKER_STATS;

typedef struct _MY_LOG_WORKER
{
    char RoleName[MY_ROLE_NAME_MAX_LEN];
    pthread_spinlock_t Lock;
    char LogPath[MY_BUFF_128];
    MY_LOG_LEVEL LogLevel;
    BOOL Inited;
    FILE* Fp;
    MY_LOG_WORKER_STATS Stats;
}
MY_LOG_WORKER;


static MY_LOG_WORKER sg_LogWorker = {
        .RoleName = {0},
        .LogPath = {0},
        .LogLevel = MY_LOG_LEVEL_INFO,
        .Inited = FALSE,
        .Fp = NULL,
        .Stats = {.LogPrinted = 0}
    };

int
LogModuleInit(
    MY_LOG_MODULE_INIT_ARG *InitArg
    )
{
    int ret = 0;

    if (sg_LogWorker.Inited)
    {
        goto CommonReturn;
    }
    
    if (!InitArg|| !InitArg->LogFilePath || !InitArg->RoleName)
    {
        ret = MY_EINVAL;
        LogErr("Too long role name!");
        goto CommonReturn;
    }
    strncpy(sg_LogWorker.RoleName, InitArg->RoleName, sizeof(sg_LogWorker.RoleName));
    strncpy(sg_LogWorker.LogPath, InitArg->LogFilePath, sizeof(sg_LogWorker.LogPath));
    sg_LogWorker.LogLevel = InitArg->LogLevel;
    pthread_spin_init(&sg_LogWorker.Lock, PTHREAD_PROCESS_PRIVATE);

    sg_LogWorker.Fp = fopen(sg_LogWorker.LogPath, "a");
    if (!sg_LogWorker.Fp)
    {
        ret = MY_EIO;
        LogErr("Open file failed!");
        goto CommonReturn;
    }
    sg_LogWorker.Inited = TRUE;
    
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

    if (level < (int)sg_LogWorker.LogLevel)
    {
        return;
    }

    if (!sg_LogWorker.Inited)
    {
        va_start(args, Fmt);
        vprintf(Fmt, args);
        va_end(args);
        printf("\n");
        return;
    }
    
    pthread_spin_lock(&sg_LogWorker.Lock);
    
    va_start(args, Fmt);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);
    char timestamp[24] = {0};
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d_%H:%M:%S", tm_info);
    int milliseconds = tv.tv_usec / 1000;
    fprintf(sg_LogWorker.Fp, "[%s.%03d][%s][%s-%d]%s:", timestamp, milliseconds, sg_LogLevelStr[level], Function, Line, sg_LogWorker.RoleName);
    vfprintf(sg_LogWorker.Fp, Fmt, args);
    va_end(args);
    fprintf(sg_LogWorker.Fp, "\n");
    fflush(sg_LogWorker.Fp);

    sg_LogWorker.Stats.LogPrinted ++;
    sg_LogWorker.Stats.LogSize = ftell(sg_LogWorker.Fp);
    
    pthread_spin_unlock(&sg_LogWorker.Lock);
    
    if (sg_LogWorker.Stats.LogSize >= MY_LOG_MEX_LEN)
    {
        pthread_spin_lock(&sg_LogWorker.Lock);
        fprintf(sg_LogWorker.Fp, "Lograting!\n");
        fsync(fileno(sg_LogWorker.Fp));
        fclose(sg_LogWorker.Fp);
        char cmd[128] = {0};
        if (strlen(sg_LogWorker.LogPath))
        {
            sprintf(cmd, "mv %s %s.0", sg_LogWorker.LogPath, sg_LogWorker.LogPath);
        }
        system(cmd);
        sg_LogWorker.Fp = fopen(sg_LogWorker.LogPath, "a");
        pthread_spin_unlock(&sg_LogWorker.Lock);
    }
}

void
LogModuleExit(
    void
    )
{
    if (sg_LogWorker.Inited)
    {
        sg_LogWorker.Inited = FALSE;
        pthread_spin_lock(&sg_LogWorker.Lock);
        pthread_spin_unlock(&sg_LogWorker.Lock);
        pthread_spin_destroy(&sg_LogWorker.Lock);
        fsync(fileno(sg_LogWorker.Fp));
        fclose(sg_LogWorker.Fp);  //we do not close fp
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
    int len = 0;
    
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
        "<%s:[LogPrinted:%d, LogSize:%lld Bytes]>", ModuleNameByEnum(MY_MODULES_ENUM_LOG), 
        sg_LogWorker.Stats.LogPrinted, sg_LogWorker.Stats.LogSize);
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
    return ret;
}

void
LogSetLevel(
    uint32_t LogLevel
    )
{
    if (LogLevel <= MY_LOG_LEVEL_ERROR)
    {
        sg_LogWorker.LogLevel = LogLevel;
    }
}
