#ifndef _MY_CMD_LINE_H_
#define _MY_CMD_LINE_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_SERVER_CMD_LINE_PORT                             15001
#define MY_CLIENT_CMD_LINE_PORT                             15002

typedef void (*ExitHandle)(void);

typedef struct _MY_CMDLINE_MODULE_INIT_ARG
{
    char* RoleName;
    int Argc;
    char** Argv;
    ExitHandle ExitFunc;
}
MY_CMDLINE_MODULE_INIT_ARG;

int
CmdLineModuleInit(
    MY_CMDLINE_MODULE_INIT_ARG *InitArg
    );

void
CmdLineModuleExit(
    void
    );

int
CmdLineModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

#ifdef __cplusplus
 }
#endif

#endif /* _MY_CMD_LINE_H_ */
