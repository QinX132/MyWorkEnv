#ifndef _MY_CMD_LINE_H_
#define _MY_CMD_LINE_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*ExitHandle)(void);

typedef struct _MY_CMDLINE_MODULE_INIT_ARG
{
    char* RoleName;
    int Argc;
    char** Argv;
    ExitHandle ExitFunc;
}
MY_CMDLINE_MODULE_INIT_ARG;

typedef enum _MY_CMDLINE_ROLE
{
    MY_CMDLINE_ROLE_SVR,
    MY_CMDLINE_ROLE_CLT,
    
    MY_CMDLINE_ROLE_UNUSED
}
MY_CMDLINE_ROLE;

typedef enum _MY_TEST_CMD_TYPE
{
    MY_TEST_CMD_TYPE_START,
    MY_TEST_CMD_TYPE_SHOWSTAT,
    MY_TEST_CMD_TYPE_STOP,
    MY_TEST_CMD_TYPE_HELP,
    MY_TEST_CMD_TYPE_CHANGE_TPOOL_TIMEOUT,
    MY_TEST_CMD_TYPE_CHANGE_LOG_LEVEL,
    
    MY_TEST_CMD_TYPE_UNUSED
}
MY_TEST_CMD_TYPE;

int
CmdLineModuleInit(
    MY_CMDLINE_MODULE_INIT_ARG *InitArg
    );

void
CmdLineModuleExit(
    void
    );

#ifdef __cplusplus
 }
#endif

#endif /* _MY_CMD_LINE_H_ */
