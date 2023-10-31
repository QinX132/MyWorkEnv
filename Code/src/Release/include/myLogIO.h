#ifndef _MY_LOG_IO_H_
#define _MY_LOG_IO_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_TEST_ROLE_NAME_MAX_LEN               128
#define MY_TEST_LOG_FILE                        "/tmp/mylog.log"

#define LogInfo(Fmt, ...)               LogPrint(MY_TEST_LOG_LEVEL_INFO, __func__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogDbg(Fmt, ...)                LogPrint(MY_TEST_LOG_LEVEL_DEBUG, __func__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogWarn(Fmt, ...)               LogPrint(MY_TEST_LOG_LEVEL_WARNING, __func__, __LINE__, Fmt, ##__VA_ARGS__)
#define LogErr(Fmt, ...)                LogPrint(MY_TEST_LOG_LEVEL_ERROR, __func__, __LINE__, Fmt, ##__VA_ARGS__)
#define debug                           LogDbg("DEBUG: [tid %u]", syscall(SYS_gettid));

typedef enum _MY_TEST_LOG_LEVEL
{
    MY_TEST_LOG_LEVEL_INFO,
    MY_TEST_LOG_LEVEL_DEBUG,
    MY_TEST_LOG_LEVEL_WARNING,
    MY_TEST_LOG_LEVEL_ERROR,
    
    MY_TEST_LOG_LEVEL_MAX
}
MY_TEST_LOG_LEVEL;

typedef struct _MY_LOG_MODULE_INIT_ARG
{
    char *LogFilePath;
    MY_TEST_LOG_LEVEL LogLevel;
    char *RoleName;
}
MY_LOG_MODULE_INIT_ARG;

int
LogModuleInit(
    MY_LOG_MODULE_INIT_ARG *InitArg
    );

void
LogPrint(
    int level,
    const char* Function,
    int Line,
    const char* Fmt,
    ...
    );

void
LogModuleExit(
    void
    );

int
LogModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

void
LogSetLevel(
    uint32_t LogLevel
    );

#ifdef __cplusplus
 }
#endif

#endif /* _MY_LOG_IO_H_ */
