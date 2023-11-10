#ifndef _MY_MSG_H_
#define _MY_MSG_H_
#include "include.h"
#include "myModuleCommon.h"

#define MY_TEST_SESSION_ID_MAX_LEN          64
#define MY_TEST_DISCONNECT_STRING           "disconnect"
#define MY_TEST_MSX_CONTENT_LEN             128
#define MY_TEST_MSG_CONTENT_MAX_LEN         1024*1024

typedef enum _MY_TEST_MSG_TYPE{
    MY_TEST_MSG_TYPE_UNUSED,
    MY_TEST_MSG_TYPE_FIRST_MSG,
    MY_TEST_MSG_TYPE_SAY_HELLO_TO_ME,
    MY_TEST_MSG_TYPE_SHOW_SESSION_TO_ME,
    MY_TEST_MSG_TYPE_ECHO_CONT_TO_ME,
    MY_TEST_MSG_TYPE_SHOW_CMD_REPLY_TO_ME,
    
    MY_TEST_MSG_TYPE_MAX
}
MY_TEST_MSG_TYPE;

typedef struct _MY_TEST_MSG_HEAD{
    uint64_t MsgId;
    uint32_t MsgContentLen;
    uint32_t MagicVer;
    uint32_t SessionId;
    uint8_t Reserved[64];
}
MY_TEST_MSG_HEAD;

typedef struct _MY_TEST_MSG_Cont{
    uint8_t VarLenCont[MY_TEST_MSG_CONTENT_MAX_LEN];
}
MY_TEST_MSG_CONT, *MY_TEST_MSG_CONT_PTR;

typedef struct _MY_TEST_MSG_TAIL{
    uint64_t TimeStamp;
    uint8_t Sign[64];
    uint8_t Reserved[64];
}
MY_TEST_MSG_TAIL;

typedef struct _MY_TEST_MSG{
    MY_TEST_MSG_HEAD Head;
    MY_TEST_MSG_CONT Cont;
    MY_TEST_MSG_TAIL Tail;
}
MY_TEST_MSG;

void
MsgModuleInit(
    void
    );

void
MsgModuleExit(
    void
    );

int
MsgModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    );

int
RecvMsg(
    int Fd,
    __inout MY_TEST_MSG * RetMsg
    );

MUST_CHECK
MY_TEST_MSG *
NewMsg(
    void
    );

void
FreeMsg(
    MY_TEST_MSG *Msg
    );

int
SendMsg(
    int Fd,
    MY_TEST_MSG Msg
    );

#endif /* _MY_MSG_H_ */
