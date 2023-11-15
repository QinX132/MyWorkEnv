#ifndef _MY_MSG_H_
#define _MY_MSG_H_
#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_MSG_CONTENT_MAX_LEN                                  (1024*1024)
#define MY_MSG_VER_MAGIC                                        0x10

// total 40 Bytes
typedef struct _MY_MSG_HEAD{
    uint8_t VerMagic;
    uint8_t IsMsgEnd;
    uint16_t ContentLen;
    uint32_t SessionId;
    uint16_t Type;
    uint8_t Reserved[30];
}
MY_MSG_HEAD;

typedef struct _MY_MSG_CONT
{
    uint8_t VarLenCont[MY_MSG_CONTENT_MAX_LEN];
}
MY_MSG_CONT;

// total 88 Bytes
typedef struct _MY_MSG_TAIL
{
    uint8_t Reserved[16];
    uint64_t TimeStamp;
    uint8_t Sign[64];
}
MY_MSG_TAIL;
// head + tail = 128 Bytes

typedef struct _MY_MSG
{
    MY_MSG_HEAD Head;
    MY_MSG_CONT Cont;
    MY_MSG_TAIL Tail;
}
MY_MSG;

int
MsgModuleInit(
    void
    );

int
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
    __inout MY_MSG *RetMsg
    );

MUST_CHECK
MY_MSG *
NewMsg(
    void
    );

void
FreeMsg(
    MY_MSG *Msg
    );

int
SendMsg(
    int Fd,
    MY_MSG *Msg
    );

int
FillMsgCont(
    MY_MSG *Msg,
    void* FillCont,
    size_t FillContLen
    );

void
ClearMsgCont(
    MY_MSG *Msg
    );

#ifdef __cplusplus
}
#endif

#endif /* _MY_MSG_H_ */
