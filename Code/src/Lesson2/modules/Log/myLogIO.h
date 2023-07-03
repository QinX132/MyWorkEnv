#ifndef _MY_LOG_IO_H_
#define _MY_LOG_IO_H_

#include "include.h"

#define MY_TEST_ROLE_NAME_MAX_LEN               128
#define MY_TEST_LOG_FILE                        "MyLog.log"

#if 1
#define LogInfo(...)        LogPrint(MY_TEST_LOG_LEVEL_INFO, __VA_ARGS__)
#define LogDbg(...)         LogPrint(MY_TEST_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LogWarn(...)        LogPrint(MY_TEST_LOG_LEVEL_WARNING, __VA_ARGS__)
#define LogErr(...)         LogPrint(MY_TEST_LOG_LEVEL_ERROR, __VA_ARGS__)
#else
#define LogInfo        printf
#define LogDbg         printf
#define LogWarn        printf
#define LogErr         printf
#endif

typedef enum _MY_TEST_LOG_LEVEL
{
    MY_TEST_LOG_LEVEL_INFO,
    MY_TEST_LOG_LEVEL_DEBUG,
    MY_TEST_LOG_LEVEL_WARNING,
    MY_TEST_LOG_LEVEL_ERROR,
    
    MY_TEST_LOG_LEVEL_MAX
}
MY_TEST_LOG_LEVEL;

int
LogModuleInit(
    char *LogFilePath,
    char *RoleName,
    size_t RoleNameLen
    );

void
LogPrint(
    int level,
    const char* Fmt,
    ...
    );

void
LogModuleExit(
    void
    );

#endif /* _MY_LOG_IO_H_ */
