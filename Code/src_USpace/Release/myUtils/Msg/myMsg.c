#include "myMsg.h"
#include "myLogIO.h"
#include "myModuleHealth.h"

typedef struct _MY_MSG_STATS
{
    uint32_t MsgSend;
    uint64_t MsgSendBytes;
    uint32_t MsgSendFailed;
    uint32_t MsgRecv;
    uint64_t MsgRecvBytes;
    uint32_t MsgRecvFailed;
    pthread_spinlock_t Lock;
    BOOL Inited;
}
MY_MSG_STATS;

static MY_MSG_STATS sg_MsgStats = {.Inited = FALSE};
static int32_t sg_MsgModId = MY_MEM_MODULE_INVALID_ID;

static void*
_MsgCalloc(
    size_t Size
    )
{
    return MemCalloc(sg_MsgModId, Size);
}

static void
_MsgFree(
    void* Ptr
    )
{
    return MemFree(sg_MsgModId, Ptr);
}

int
MsgModuleInit(
    void
    )
{
    int ret = MY_SUCCESS;
    if (! sg_MsgStats.Inited)
    {
        ret = MemRegister(&sg_MsgModId, "Msg");
        if (ret)
        {
            goto CommonReturn;
        }
        memset(&sg_MsgStats, 0, sizeof(sg_MsgStats));
        pthread_spin_init(&sg_MsgStats.Lock, PTHREAD_PROCESS_PRIVATE);
        sg_MsgStats.Inited = TRUE;
    }
CommonReturn:
    return ret;
}

int
MsgModuleExit(
    void
    )
{
    int ret = MY_SUCCESS;
    
    if (sg_MsgStats.Inited)
    {
        ret = MemUnRegister(&sg_MsgModId);
        pthread_spin_lock(&sg_MsgStats.Lock);
        pthread_spin_unlock(&sg_MsgStats.Lock);
        pthread_spin_destroy(&sg_MsgStats.Lock);
    }

    return ret;
}

int
MsgModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = 0;
    int len = 0;
    
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
        "<%s:[MsgSend=%u, MsgSendBytes=%llu, MsgSendFailed=%u, MsgRecv=%u, MsgRecvBytes=%llu, MsgRecvFailed=%u]>",
        ModuleNameByEnum(MY_MODULES_ENUM_MSG), sg_MsgStats.MsgSend, sg_MsgStats.MsgSendBytes, 
        sg_MsgStats.MsgSendFailed, sg_MsgStats.MsgRecv, sg_MsgStats.MsgRecvBytes, sg_MsgStats.MsgRecvFailed);
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }
    
CommonReturn:
    return ret;
}

int
RecvMsg(
    int Fd,
    __inout MY_MSG *RetMsg
    )
{
    int ret = 0;
    int recvLen = 0;
    int currentLen = 0;
    int recvRet = 0;
    char recvLogBuff[MY_MSG_CONTENT_MAX_LEN + MY_BUFF_1024] = {0};
    size_t recvLogLen = 0;
    size_t len = 0;

    if (!sg_MsgStats.Inited)
    {
        goto CommonReturn;
    }

    if (!RetMsg)
    {
        ret = MY_EINVAL;
        LogErr("NULL ptr!");
        goto CommonReturn;
    }
    
    memset(RetMsg, 0, sizeof(MY_MSG));
    // recv head
    recvLen = sizeof(MY_MSG_HEAD);
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)RetMsg) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
#ifdef DEBUG
            LogInfo("recvRet = %d, HeadLen=%d", recvRet, sizeof(MY_MSG_HEAD));
#endif 
        }
        else if (recvRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            else
            {
                ret = errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "Recv Msg: MsgType=%u ContentLen=%u VerMagic=%u, SessionId=%u, IsMsgEnd=%u ", 
                RetMsg->Head.Type, RetMsg->Head.ContentLen, RetMsg->Head.VerMagic, RetMsg->Head.SessionId,
                RetMsg->Head.IsMsgEnd);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;
    // recv content
    recvLen = RetMsg->Head.ContentLen;
    if (UNLIKELY(recvLen > (int)sizeof(MY_MSG_CONT)))
    {
        LogErr("Too long cont len %u", RetMsg->Head.ContentLen);
        ret = EINVAL;
        goto CommonReturn;
    }
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)RetMsg + sizeof(MY_MSG_HEAD)) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
#ifdef DEBUG
            LogInfo("recvRet = %d, ContLen=%d", recvRet, RetMsg->Head.ContentLen);
#endif 
        }
        else if (recvRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            else
            {
                ret = errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "VarLenCont=\"%s\" ", RetMsg->Cont.VarLenCont);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;
    // recv tail
    recvLen = sizeof(MY_MSG_TAIL);
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)RetMsg + sizeof(MY_MSG_HEAD) + sizeof(MY_MSG_CONT)) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
#ifdef DEBUG
            LogInfo("recvRet = %d, TailLen=%d", recvRet, sizeof(MY_MSG_TAIL));
#endif 
        }
        else if (recvRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            else
            {
                ret = errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "TimeStamp=%llu", RetMsg->Tail.TimeStamp);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;

    LogInfo("%s", recvLogBuff);
CommonReturn:
    if (sg_MsgStats.Inited)
    {
        pthread_spin_lock(&sg_MsgStats.Lock);
        if (ret != 0 && ret != MY_ERR_PEER_CLOSED)
        {
            sg_MsgStats.MsgRecvFailed ++;
        }
        else
        {
            sg_MsgStats.MsgRecv ++;
            sg_MsgStats.MsgRecvBytes += sizeof(MY_MSG_HEAD) + RetMsg->Head.ContentLen + sizeof(MY_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgStats.Lock);
    }
    return ret;
}

MUST_CHECK
MY_MSG *
NewMsg(
    void
    )
{
    MY_MSG* retMsg = NULL;
    static uint32_t sessionId = 0;
    
    if (!sg_MsgStats.Inited)
    {
        goto CommonReturn;
    }
    
    retMsg = (MY_MSG*)_MsgCalloc(sizeof(MY_MSG));
    if (retMsg)
    {
        retMsg->Head.VerMagic = MY_MSG_VER_MAGIC;
        pthread_spin_lock(&sg_MsgStats.Lock);
        retMsg->Head.SessionId = sessionId ++;
        pthread_spin_unlock(&sg_MsgStats.Lock);
    }

CommonReturn:
    return retMsg;
}

void
FreeMsg(
    MY_MSG *Msg
    )
{
    if (Msg)
    {
        _MsgFree(Msg);
        Msg = NULL;
    }
}

int
SendMsg(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = 0;
    int sendLen = 0;
    int currentLen = 0;
    int sendRet = 0;
    char sendLogBuff[MY_MSG_CONTENT_MAX_LEN + MY_BUFF_1024] = {0};
    size_t sendLogLen = 0;
    size_t len = 0;
    struct timeval tv;
    
    if (!sg_MsgStats.Inited || !Msg)
    {
        goto CommonReturn;
    }
    
    gettimeofday(&tv, NULL);
    Msg->Tail.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;
    //send msg header
    sendLen = sizeof(MY_MSG_HEAD);
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg->Head) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
#ifdef DEBUG
            LogInfo("sendRet = %d, HeadLen=%d", sendRet, sizeof(MY_MSG_HEAD));
#endif 
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "Send Msg: MsgType=%u ContentLen=%u VerMagic=%u, SessionId=%u, IsMsgEnd=%u ", 
                Msg->Head.Type, Msg->Head.ContentLen, Msg->Head.VerMagic, Msg->Head.SessionId,
                Msg->Head.IsMsgEnd);
    if (len >= sizeof(sendLogBuff) - sendLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    sendLogLen += len;
    // send msg content
    sendLen = Msg->Head.ContentLen;
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg->Head + sizeof(MY_MSG_HEAD)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
#ifdef DEBUG
            LogInfo("sendRet = %d, ContLen=%d", sendRet, Msg->Head.ContentLen);
#endif 
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "VarLenCont=\"%s\" ", Msg->Cont.VarLenCont);
    if (len >= sizeof(sendLogBuff) - sendLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    sendLogLen += len;
    // send msg tail
    sendLen = sizeof(MY_MSG_TAIL);
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg->Head + sizeof(MY_MSG_HEAD) + sizeof(MY_MSG_CONT)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
#ifdef DEBUG
            LogInfo("sendRet = %d, TailLen=%d", sendRet, sizeof(MY_MSG_TAIL));
#endif 
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "TimeStamp=%llu", Msg->Tail.TimeStamp);
    if (len >= sizeof(sendLogBuff) - sendLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    sendLogLen += len;

    LogInfo("%s", sendLogBuff);

CommonReturn:
    if (sg_MsgStats.Inited)
    {
        pthread_spin_lock(&sg_MsgStats.Lock);
        if (ret != 0)
        {
            sg_MsgStats.MsgSendFailed ++;
        }
        else
        {
            sg_MsgStats.MsgSend ++;
            sg_MsgStats.MsgSendBytes += sizeof(MY_MSG_HEAD) + Msg->Head.ContentLen + sizeof(MY_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgStats.Lock);
    }
    return ret;
}

int
FillMsgCont(
    MY_MSG *Msg,
    void* FillCont,
    size_t FillContLen
    )
{
    int ret = MY_SUCCESS;
    if (!Msg || !FillCont || !FillContLen)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }

    if (FillContLen >= sizeof(Msg->Cont.VarLenCont) - Msg->Head.ContentLen)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }

    memcpy(Msg->Cont.VarLenCont + Msg->Head.ContentLen, FillCont, FillContLen);
    Msg->Head.ContentLen += FillContLen;

CommonReturn:
    return ret;
}

void
ClearMsgCont(
    MY_MSG *Msg
    )
{
    if (Msg)
    {
        Msg->Head.ContentLen = 0;
        memset(Msg->Cont.VarLenCont, 0, sizeof(Msg->Cont.VarLenCont));
    }
}
