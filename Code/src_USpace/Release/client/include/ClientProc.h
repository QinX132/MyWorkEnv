#ifndef _CLIENT_PROC_H_
#define _CLIENT_PROC_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "myModuleCommon.h"
#define MY_CLIENT_ROLE_NAME                                 "QinXClient"

typedef struct _CLIENT_CONF_PARAM
{
    char ServerIp[MY_BUFF_64];
    MY_LOG_LEVEL LogLevel;
    char LogFilePath[MY_BUFF_64];
    int LogMaxSize;
    int LogMaxNum;
}
CLIENT_CONF_PARAM;

int
ClientProcInit(
    CLIENT_CONF_PARAM *ClientConfParam
    );

void
ClientProcExit(
    void
    );

void
ClientProcMainLoop(
    void
    );

#ifdef __cplusplus
}
#endif

#endif /* _CLIENT_PROC_H_ */

