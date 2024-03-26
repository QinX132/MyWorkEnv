#include "myMsg.h"
#include "myLogIO.h"
#include "myModuleHealth.h"
#include "myCommonUtil.h"

_Static_assert(sizeof(MY_MSG_HEAD) == 40, "msg head size mismatch!");
_Static_assert(sizeof(MY_MSG_TAIL) == 88, "msg tail size mismatch!");

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
    int ret = MY_SUCCESS;
    int len = 0;
    
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
        "<%s:[MsgSend=%u, MsgSendBytes=%"PRIu64", MsgSendFailed=%u, MsgRecv=%u, MsgRecvBytes=%"PRIu64", MsgRecvFailed=%u]>",
        ModuleNameByEnum(MY_MODULES_ENUM_MSG), sg_MsgStats.MsgSend, sg_MsgStats.MsgSendBytes, 
        sg_MsgStats.MsgSendFailed, sg_MsgStats.MsgRecv, sg_MsgStats.MsgRecvBytes, sg_MsgStats.MsgRecvFailed);
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = -MY_ENOMEM;
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
    int ret = MY_SUCCESS;
    int recvLen = 0;
    int currentLen = 0, tmpLen = 0;
    int recvRet = 0;
    char recvLogBuff[MY_MSG_CONTENT_MAX_LEN + MY_BUFF_1024] = {0};
    size_t recvLogLen = 0;
    struct timeval tv;
    int64_t latency = 0;

    if (!sg_MsgStats.Inited)
    {
        goto CommonReturn;
    }

    if (!RetMsg)
    {
        ret = -MY_EINVAL;
        LogErr("NULL ptr!");
        goto CommonReturn;
    }
    
    gettimeofday(&tv, NULL);
    memset(RetMsg, 0, sizeof(MY_MSG));
    // recv head
    currentLen = 0;
    recvLen = sizeof(MY_MSG_HEAD);
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
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
                ret = -errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    RetMsg->Head.Type = ntohs(RetMsg->Head.Type);
    RetMsg->Head.ContentLen = ntohl(RetMsg->Head.ContentLen);
    RetMsg->Head.SessionId = ntohl(RetMsg->Head.SessionId);
    RetMsg->Head.ClientId = ntohl(RetMsg->Head.ClientId);
    recvLogLen += snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "Recv Msg: VerMagic=0x%x ClientId=%u SessionId=%u MsgType=%u ContentLen=%u IsMsgEnd=%u ", 
                RetMsg->Head.VerMagic, RetMsg->Head.ClientId, RetMsg->Head.Type, RetMsg->Head.SessionId,
                RetMsg->Head.ContentLen, RetMsg->Head.IsMsgEnd);
    // recv content
    currentLen = 0;
    recvLen = RetMsg->Head.ContentLen;
    if (UNLIKELY(recvLen > (int)sizeof(MY_MSG_CONT)))
    {
        LogErr("Too long cont len %u", RetMsg->Head.ContentLen);
        ret = -MY_EINVAL;
        goto CommonReturn;
    }
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
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
                ret = -errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    tmpLen = recvLogLen;
    recvLogLen += snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "VarLenCont=\"%s\" ", RetMsg->Cont.VarLenCont);
    MyUtil_ChangeCharA2B(recvLogBuff + tmpLen, recvLogLen - tmpLen, '\n', ' ');
    // recv tail
    currentLen = 0;
    recvLen = sizeof(MY_MSG_TAIL);
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
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
                ret = -errno;
                LogErr("recvRet = %d, %d:%s", recvRet, errno, My_StrErr(errno));
                goto CommonReturn;
            }
        }
    }
    RetMsg->Tail.TimeStamp = MyUtil_ntohll(RetMsg->Tail.TimeStamp);
    latency = (int64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000 - RetMsg->Tail.TimeStamp);
    recvLogLen += snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "TimeStamp=%"PRIu64" Latency=%"PRId64" ms", RetMsg->Tail.TimeStamp, 
                latency > 0 ? latency : 0);
    LogInfo("%s", recvLogBuff);
CommonReturn:
    if (sg_MsgStats.Inited)
    {
        pthread_spin_lock(&sg_MsgStats.Lock);
        if (ret != MY_SUCCESS && ret != -MY_ERR_PEER_CLOSED)
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
    
    if (!sg_MsgStats.Inited)
    {
        goto CommonReturn;
    }
    
    retMsg = (MY_MSG*)_MsgCalloc(sizeof(MY_MSG));
    if (retMsg)
    {
        retMsg->Head.VerMagic = MY_MSG_VER_MAGIC;
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
    int currentLen = 0, tmpLen = 0;;
    int sendRet = 0;
    char sendLogBuff[MY_MSG_CONTENT_MAX_LEN + MY_BUFF_1024] = {0};
    size_t sendLogLen = 0;
    struct timeval tv;
    
    if (!sg_MsgStats.Inited || !Msg)
    {
        goto CommonReturn;
    }
    
    Msg->Head.IsMsgEnd  = TRUE;
    //send msg header
    sendLen = sizeof(MY_MSG_HEAD);
    currentLen = 0;
    Msg->Head.Type = htons(Msg->Head.Type);
    Msg->Head.ContentLen = htonl(Msg->Head.ContentLen);
    Msg->Head.SessionId = htonl(Msg->Head.SessionId);
    Msg->Head.ClientId = htonl(Msg->Head.ClientId);
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            ret = -errno;
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    Msg->Head.Type = ntohs(Msg->Head.Type);
    Msg->Head.ContentLen = ntohl(Msg->Head.ContentLen);
    Msg->Head.SessionId = ntohl(Msg->Head.SessionId);
    Msg->Head.ClientId = ntohl(Msg->Head.ClientId);
    sendLogLen += snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "Send Msg: VerMagic=0x%x ClientId=%u SessionId=%u MsgType=%u ContentLen=%u IsMsgEnd=%u ", 
                Msg->Head.VerMagic, Msg->Head.ClientId, Msg->Head.SessionId, Msg->Head.Type,
                Msg->Head.ContentLen, Msg->Head.IsMsgEnd);
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            ret = -errno;
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    tmpLen = sendLogLen;
    sendLogLen += snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "VarLenCont=\"%s\" ", Msg->Cont.VarLenCont);
    MyUtil_ChangeCharA2B(sendLogBuff + tmpLen, sendLogLen - tmpLen, '\n', ' ');
    // send msg tail
    gettimeofday(&tv, NULL);
    Msg->Tail.TimeStamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    sendLen = sizeof(MY_MSG_TAIL);
    currentLen = 0;
    Msg->Tail.TimeStamp = MyUtil_htonll(Msg->Tail.TimeStamp);
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
            ret = -MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                // should retry
                continue;
            }
            ret = -errno;
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    Msg->Tail.TimeStamp = MyUtil_ntohll(Msg->Tail.TimeStamp);
    sendLogLen += snprintf(sendLogBuff + sendLogLen, sizeof(sendLogBuff) - sendLogLen, 
                "TimeStamp=%"PRIu64"", Msg->Tail.TimeStamp);
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
        ret = -MY_EINVAL;
        goto CommonReturn;
    }

    if (FillContLen >= sizeof(Msg->Cont.VarLenCont) - Msg->Head.ContentLen)
    {
        ret = -MY_ENOMEM;
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
