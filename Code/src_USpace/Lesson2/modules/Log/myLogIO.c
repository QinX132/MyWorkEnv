#include "myLogIO.h"

static FILE* sg_LogFileFp = NULL;
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
    va_list args;
    pthread_spin_lock(&sg_LogSpinlock);
    
    sg_LogFileFp = fopen(sg_LogPath, "a+");
    
    va_start(args, Fmt);
    time(&timep);
    ptm = localtime(&timep);
    fprintf(sg_LogFileFp, "%s[%04d-%02d-%02d-%02d:%02d:%02d]<%5s>:", sg_RoleName, ptm->tm_year + 1900, 
        ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, sg_LogLevelStr[level]);
    vfprintf(sg_LogFileFp, Fmt, args);
    va_end(args);
    fprintf(sg_LogFileFp, "\n");
    fsync(fileno(sg_LogFileFp));
    
    fclose(sg_LogFileFp);
    
    pthread_spin_unlock(&sg_LogSpinlock);
}

void
LogModuleExit(
    void
    )
{
    pthread_spin_lock(&sg_LogSpinlock);
    pthread_spin_unlock(&sg_LogSpinlock);
    pthread_spin_destroy(&sg_LogSpinlock);
}
