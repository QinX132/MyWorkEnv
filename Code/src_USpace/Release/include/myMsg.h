#ifndef _MY_MSG_H_
#define _MY_MSG_H_
#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MY_MSG_CONTENT_MAX_LEN                                  (1024 * 1024)
#define MY_MSG_VER(_M_VER_, _F_VER_, _R_VER_)                   (_M_VER_ << 5 | _F_VER_ << 2 | _R_VER_)
                                                    // m_ver 3bits max 7, f_ver 2bits max 3, r_ver 3 bits max 7
#define MY_MSG_VER_MAGIC                                        MY_MSG_VER(1, 0, 0)

// total 40 Bytes
typedef struct _MY_MSG_HEAD{
    uint8_t VerMagic;               // must be the first
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t IsMsgEnd : 1;
    uint8_t Reserved_1 : 7;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint8_t Reserved_1 : 7;
    uint8_t IsMsgEnd : 1;
#else
    #error "unknown byte order!"
#endif
    uint16_t Type;
    uint32_t ContentLen;
    uint32_t SessionId;
    uint32_t ClientId;
    uint8_t Reserved_2[24];
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
    uint64_t TimeStamp;         // ms timestamp
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
