#include "myMsg.h"
#include "myLogIO.h"
#include "myModuleHealth.h"

static uint32_t MsgSend = 0;
static uint64_t MsgSendBytes = 0;
static uint32_t MsgSendFailed = 0;
static uint32_t MsgRecv = 0;
static uint64_t MsgRecvBytes = 0;
static uint32_t MsgRecvFailed = 0;
static pthread_spinlock_t sg_MsgSpinlock;
static BOOL sg_MsgModuleInited = FALSE;

void
MsgModuleInit(
    void
    )
{
    pthread_spin_init(&sg_MsgSpinlock, PTHREAD_PROCESS_PRIVATE);
    sg_MsgModuleInited = TRUE;
}

void
MsgModuleExit(
    void
    )
{
    if (sg_MsgModuleInited)
    {
        pthread_spin_lock(&sg_MsgSpinlock);
        pthread_spin_unlock(&sg_MsgSpinlock);
        pthread_spin_destroy(&sg_MsgSpinlock);
    }
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
        ModuleNameByEnum(MY_MODULES_ENUM_MSG), MsgSend, MsgSendBytes, MsgSendFailed, MsgRecv, MsgRecvBytes, MsgRecvFailed);
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
    __inout MY_MSG * RetMsg
    )
{
    int ret = 0;
    int recvLen = 0;
    int currentLen = 0;
    int recvRet = 0;
    char recvLogBuff[MY_BUFF_1024] = {0};
    size_t recvLogLen = 0;
    size_t len = 0;

    if (!sg_MsgModuleInited)
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
                "Recv Msg: MsgId=%llu MsgContentLen=%u MagicVer=%u, SessionId=%u ", 
                RetMsg->Head.MsgId, RetMsg->Head.MsgContentLen, RetMsg->Head.MagicVer, RetMsg->Head.SessionId);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;
    // recv content
    recvLen = RetMsg->Head.MsgContentLen;
    if (unlikely(recvLen > (int)sizeof(MY_MSG_CONT)))
    {
        LogErr("Too long cont len %u", RetMsg->Head.MsgContentLen);
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
    if (sg_MsgModuleInited)
    {
        pthread_spin_lock(&sg_MsgSpinlock);
        if (ret != 0)
        {
            MsgRecvFailed ++;
        }
        else
        {
            MsgRecv ++;
            MsgRecvBytes += sizeof(MY_MSG_HEAD) + RetMsg->Head.MsgContentLen + sizeof(MY_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgSpinlock);
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
    
    if (!sg_MsgModuleInited)
    {
        goto CommonReturn;
    }
    
    retMsg = (MY_MSG*)MyCalloc(sizeof(MY_MSG));

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
        MyFree(Msg);
        Msg = NULL;
    }
}

int
SendMsg(
    int Fd,
    MY_MSG Msg
    )
{
    int ret = 0;
    int sendLen = 0;
    int currentLen = 0;
    int sendRet = 0;
    char recvLogBuff[MY_BUFF_1024] = {0};
    size_t recvLogLen = 0;
    size_t len = 0;
    
    if (!sg_MsgModuleInited)
    {
        goto CommonReturn;
    }
    //send msg header
    sendLen = sizeof(MY_MSG_HEAD);
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg.Head) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "Send Msg: MsgId=%llu MsgContentLen=%u MagicVer=%u, SessionId=%u ", 
                Msg.Head.MsgId, Msg.Head.MsgContentLen, Msg.Head.MagicVer, Msg.Head.SessionId);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;
    // send msg content
    sendLen = Msg.Head.MsgContentLen;
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg.Head + sizeof(MY_MSG_HEAD)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "VarLenCont=\"%s\" ", Msg.Cont.VarLenCont);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;
    // send msg tail
    sendLen = sizeof(MY_MSG_TAIL);
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg.Head + sizeof(MY_MSG_HEAD) + sizeof(MY_MSG_CONT)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
        }
        else if (sendRet == 0)
        {
            ret = MY_ERR_PEER_CLOSED; // peer close connection
            goto CommonReturn;
        }
        else
        {
            LogErr("Send failed %d:%s", errno, My_StrErr(errno));
            goto CommonReturn;
        }
    }
    len = snprintf(recvLogBuff + recvLogLen, sizeof(recvLogBuff) - recvLogLen, 
                "TimeStamp=%llu", Msg.Tail.TimeStamp);
    if (len >= sizeof(recvLogBuff) - recvLogLen)
    {
        ret = MY_ENOBUFS;
        LogErr("Too long msg!");
        goto CommonReturn;
    }
    recvLogLen += len;

    LogInfo("%s", recvLogBuff);

CommonReturn:
    if (sg_MsgModuleInited)
    {
        pthread_spin_lock(&sg_MsgSpinlock);
        if (ret != 0)
        {
            MsgSendFailed ++;
        }
        else
        {
            MsgSend ++;
            MsgSendBytes += sizeof(MY_MSG_HEAD) + Msg.Head.MsgContentLen + sizeof(MY_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgSpinlock);
    }
    return ret;
}
