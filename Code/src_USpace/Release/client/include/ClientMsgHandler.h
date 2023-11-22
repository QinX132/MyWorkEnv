#ifndef _CLIENT_MSG_HANDLER_H_
#define _CLIENT_MSG_HANDLER_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

void
ClientCmdUsage(
    void
    );

BOOL
ClientCmdSupported(
    __in char* Cmd,
    __in size_t CmdLen,
    __out int32_t* MsgType,
    __out char* MsgCont,
    __out size_t MsgContLen
    );

BOOL
ClientCmdIsExit(
    char* Cmd
    );

int32_t 
ClientHandleCmd(
    int32_t Fd,
    int32_t MsgType,
    char* MsgCont,
    size_t MsgContLen
    );

int 
ClientHandleMsg(
    int Fd,
    MY_MSG *Msg
    );

#ifdef __cplusplus
}
#endif

#endif /* _CLIENT_MSG_HANDLER_H_ */

