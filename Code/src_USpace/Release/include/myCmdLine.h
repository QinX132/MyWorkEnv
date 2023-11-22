#ifndef _MY_CMD_LINE_H_
#define _MY_CMD_LINE_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_SERVER_CMD_LINE_PORT                             15001

typedef void (*ExitHandle)(void);
typedef int (*CmdExternalFn)(char*, size_t, char*, size_t);

typedef struct _MY_CMD_EXTERNAL_CONT
{
    char Opt[MY_BUFF_64];
    char Help[MY_BUFF_256];
    int Argc;
    CmdExternalFn Cb;
}
MY_CMD_EXTERNAL_CONT;

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

int
CmdExternalRegister(
    __in MY_CMD_EXTERNAL_CONT Cont
    );

#ifdef __cplusplus
 }
#endif

#endif /* _MY_CMD_LINE_H_ */
