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

int
LogModuleInit(
    char *LogFilePath,
    char *RoleName,
    size_t RoleNameLen
    )
{
    int ret = 0;

    sg_LogFileFp = fopen(LogFilePath, "w+");

    if (sg_LogFileFp == NULL) {
        ret = -errno;
        printf("Failed to open log file\n");
        goto CommonReturn;
    }

    if (RoleNameLen >= MY_TEST_ROLE_NAME_MAX_LEN)
    {
        ret = -1;
        printf("Too long role name!\n");
        goto CommonReturn;
    }
    strncpy(sg_RoleName, RoleName, RoleNameLen);

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
    va_list args;
    va_start(args, Fmt);
    
    fprintf(sg_LogFileFp, "%s <%s>:", sg_RoleName, sg_LogLevelStr[level]);
    vfprintf(sg_LogFileFp, Fmt, args);
    va_end(args);
    fprintf(sg_LogFileFp, "\n");
}

void
LogModuleExit(
    void
    )
{
    if (sg_LogFileFp)
    {
        fclose(sg_LogFileFp);
    }
}
