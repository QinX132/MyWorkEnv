#include "myLogIO.h"

static char sg_RoleName[MY_TEST_ROLE_NAME_MAX_LEN] = {0};
static char *sg_LogLevelStr [MY_TEST_LOG_LEVEL_MAX] = 
{
    [MY_TEST_LOG_LEVEL_INFO] = "Info",
    [MY_TEST_LOG_LEVEL_DEBUG] = "Debug",
    [MY_TEST_LOG_LEVEL_WARNING] = "Warn",
    [MY_TEST_LOG_LEVEL_ERROR] = "Error",
};
static pthread_spinlock_t sg_LogSpinlock;
static char sg_LogPath[128] = {0};
static BOOL sg_LogModuleInited = FALSE;

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

CommonReturn:
    return ret;
}

void
LogPrint(
    int level,
    const char* Fmt,
    ...
    )
{
    time_t timep;
    struct tm *ptm = NULL;
    static FILE* logFileFp = NULL;

    if (!sg_LogModuleInited)
    {
        return;
    }
    
    va_list args;
    pthread_spin_lock(&sg_LogSpinlock);
    
    logFileFp = fopen(sg_LogPath, "a+");
    
    va_start(args, Fmt);
    time(&timep);
    ptm = localtime(&timep);
    fprintf(logFileFp, "%s[%04d-%02d-%02d-%02d:%02d:%02d]<%5s>:", sg_RoleName, ptm->tm_year + 1900, 
        ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, sg_LogLevelStr[level]);
    vfprintf(logFileFp, Fmt, args);
    va_end(args);
    fprintf(logFileFp, "\n");
    fsync(fileno(logFileFp));
    
    fclose(logFileFp);
    
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
    }
}
