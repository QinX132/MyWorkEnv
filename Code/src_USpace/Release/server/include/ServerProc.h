#ifndef _SERVER_PROC_H_
#define _SERVER_PROC_H_

#ifdef __cplusplus
extern "C"{
#endif

int
ServerProcInit(
    void
    );

void
ServerProcExit(
    void
    );

int
ServerProcCmdShowStats(
    __in char* Cmd,
    __in size_t CmdLen,
    __out char* ReplyBuff,
    __out size_t ReplyBuffLen
    );

#ifdef __cplusplus
}
#endif

#endif /* _SERVER_PROC_H_ */
