#include "myLogIO.h"
#include "myModuleHealth.h"

#define MY_TEST_LOG_MEX_LEN             (50 * 1024 * 1024)

static char sg_RoleName[MY_TEST_ROLE_NAME_MAX_LEN] = {0};
static char *sg_LogLevelStr [MY_TEST_LOG_LEVEL_MAX] = 
{
    [MY_TEST_LOG_LEVEL_INFO] = "Inf",
    [MY_TEST_LOG_LEVEL_DEBUG] = "Dbg",
    [MY_TEST_LOG_LEVEL_WARNING] = "Wrn",
    [MY_TEST_LOG_LEVEL_ERROR] = "Err",
};
static pthread_spinlock_t sg_LogSpinlock;
static char sg_LogPath[128] = {0};
static BOOL sg_LogModuleInited = FALSE;
static FILE* sg_LogFileFp = NULL;

static int sg_LogPrinted = 0;

int
LogModuleInit(
    char *LogFilePath,
    char *RoleName,
    size_t RoleNameLen
    )
{
    int ret = 0;

    if (RoleNameLen >= MY_TEST_ROLE_NAME_MAX_LEN || !LogFilePath || !RoleName)
    {
        ret = -1;
        printf("Too long role name!\n");
        goto CommonReturn;
    }
    strncpy(sg_RoleName, RoleName, RoleNameLen);
    strcpy(sg_LogPath, LogFilePath);
    pthread_spin_init(&sg_LogSpinlock, PTHREAD_PROCESS_PRIVATE);
    sg_LogModuleInited = TRUE;

    sg_LogFileFp = fopen(sg_LogPath, "a");
    
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

    if (!sg_LogModuleInited)
    {
        return;
    }
    
    va_list args;
    pthread_spin_lock(&sg_LogSpinlock);
    
    va_start(args, Fmt);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm* tm_info = localtime(&now);
    char timestamp[24] = {0};
    strftime(timestamp, sizeof(timestamp), "%Y/%m/%d_%H:%M:%S", tm_info);
    int milliseconds = tv.tv_usec / 1000;
    fprintf(sg_LogFileFp, "[%s.%03d]%s<%s>[%s:%d]:", timestamp, milliseconds, sg_RoleName, sg_LogLevelStr[level], Function, Line);
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
        fclose(sg_LogFileFp);
    }
}

void
LogModuleStat(
    evutil_socket_t Fd,
    short Event,
    void *Arg
    )
{
    long long size = 0;
    FILE* fp = fopen(sg_LogPath, "rb");
    UNUSED(Arg);
    UNUSED(Fd);
    UNUSED(Event);
    
    if (!fp) 
    {
        LogErr("Failed to open file: %s\n", sg_LogPath);
        goto CommonReturn;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);

    LogInfo("<%s:[LogPrinted:%d, LogSize:%lld]>", ModuleNameByEnum(MY_MODULES_ENUM_LOG), sg_LogPrinted, size);

CommonReturn:
    if (fp)
        fclose(fp);

    if (size >= MY_TEST_LOG_MEX_LEN)
    {
        pthread_spin_lock(&sg_LogSpinlock);
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
    return ;
}
