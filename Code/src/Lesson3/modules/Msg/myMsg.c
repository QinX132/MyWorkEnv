#include "myMsg.h"
#include "myLogIO.h"

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

void
EchoMsgModuleStats(
    void
    )
{
    LogInfo("MsgSend = %u, MsgSendBytes = %llu, MsgSendFailed = %u, MsgRecv = %u, MsgRecvBytes = %llu, MsgRecvFailed = %u",
        MsgSend, MsgSendBytes, MsgSendFailed, MsgRecv, MsgRecvBytes, MsgRecvFailed);
}

MY_TEST_MSG *
RecvMsg(
    int Fd
    )
{
    int ret = 0;
    MY_TEST_MSG *retMsg = NULL;
    int recvLen = 0;
    int currentLen = 0;
    int recvRet = 0;

    if (!sg_MsgModuleInited)
    {
        goto CommonReturn;
    }
    
    retMsg = (MY_TEST_MSG*)malloc(sizeof(MY_TEST_MSG));
    if (!retMsg)
    {
        ret = -ENOMEM;
        LogErr("Apply memory failed!");
        goto CommonReturn;
    }
    memset(retMsg, 0, sizeof(MY_TEST_MSG));
    // recv head
    recvLen = sizeof(MY_TEST_MSG_HEAD);
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)retMsg) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
        }
        else if (recvRet == 0)
        {
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                ret = -errno;
                LogErr("recv ends!");
                goto CommonReturn;
            }
            else
            {
                ret = -errno;
                LogErr("%d:%s",errno,strerror(errno));
                goto CommonReturn;
            }
        }
    }
    LogInfo("MsgId=%llu, MsgContentLen=%u, MagicVer=%u, SessionId=%u", 
        retMsg->Head.MsgId, retMsg->Head.MsgContentLen, retMsg->Head.MagicVer, retMsg->Head.SessionId);
    // recv content
    recvLen = retMsg->Head.MsgContentLen;
    if (unlikely(recvLen > (int)sizeof(MY_TEST_MSG_CONT)))
    {
        LogErr("Too long cont len %u", retMsg->Head.MsgContentLen);
        ret = EINVAL;
        goto CommonReturn;
    }
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)retMsg + sizeof(MY_TEST_MSG_HEAD)) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
        }
        else if (recvRet == 0)
        {
            goto CommonReturn;
        }
        else
        {
            LogErr("%d:%s",errno,strerror(errno));
            goto CommonReturn;
        }
    }
    LogInfo("VarLenCont=%s", retMsg->Cont.VarLenCont);
    // recv tail
    recvLen = sizeof(MY_TEST_MSG_TAIL);
    currentLen = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)retMsg + sizeof(MY_TEST_MSG_HEAD) + sizeof(MY_TEST_MSG_CONT)) + currentLen, recvLen - currentLen, 0);
        if (recvRet > 0)
        {
            currentLen += recvRet;
        }
        else if (recvRet == 0)
        {
            goto CommonReturn;
        }
        else
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                LogErr("recv ends!");
                goto CommonReturn;
            }
            else
            {
                ret = -errno;
                LogErr("%d:%s",errno,strerror(errno));
                goto CommonReturn;
            }
        }
    }
    LogInfo("TimeStamp=%llu", retMsg->Tail.TimeStamp);
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
            MsgRecvBytes += sizeof(MY_TEST_MSG_HEAD) + retMsg->Head.MsgContentLen + sizeof(MY_TEST_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgSpinlock);
    }
    return retMsg;
}

MUST_CHECK
MY_TEST_MSG *
NewMsg(
    void
    )
{
    MY_TEST_MSG* retMsg = NULL;
    
    if (!sg_MsgModuleInited)
    {
        goto CommonReturn;
    }
    
    retMsg = (MY_TEST_MSG*)malloc(sizeof(MY_TEST_MSG));
    memset(retMsg, 0, sizeof(MY_TEST_MSG));

CommonReturn:
    return retMsg;
}

void
FreeMsg(
    MY_TEST_MSG *Msg
    )
{
    if (Msg)
    {
        free(Msg);
        Msg = NULL;
    }
}

int
SendMsg(
    int Fd,
    MY_TEST_MSG Msg
    )
{
    int ret = 0;
    int sendLen = sizeof(MY_TEST_MSG_HEAD);
    int currentLen = 0;
    int sendRet = 0;
    
    if (!sg_MsgModuleInited)
    {
        goto CommonReturn;
    }
    sendLen = sizeof(MY_TEST_MSG_HEAD);
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
            ret = -errno;
            goto CommonReturn;
        }
        else
        {
            printf("Send failed %d:%s\n",errno,strerror(errno));
            goto CommonReturn;
        }
    }
    sendLen = Msg.Head.MsgContentLen;
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg.Head + sizeof(MY_TEST_MSG_HEAD)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
        }
        else if (sendRet == 0)
        {
            ret = -errno;
            goto CommonReturn;
        }
        else
        {
            printf("Send failed %d:%s\n",errno,strerror(errno));
            goto CommonReturn;
        }
    }
    sendLen = sizeof(MY_TEST_MSG_TAIL);
    currentLen = 0;
    for(; currentLen < sendLen;)
    {
        sendRet = send(Fd, ((char*)&Msg.Head + sizeof(MY_TEST_MSG_HEAD) + sizeof(MY_TEST_MSG_CONT)) + currentLen, sendLen - currentLen, 0);
        if (sendRet > 0)
        {
            currentLen += sendRet;
        }
        else if (sendRet == 0)
        {
            ret = -errno;
            goto CommonReturn;
        }
        else
        {
            printf("Send failed %d:%s\n",errno,strerror(errno));
            goto CommonReturn;
        }
    }

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
            MsgSendBytes += sizeof(MY_TEST_MSG_HEAD) + Msg.Head.MsgContentLen + sizeof(MY_TEST_MSG_TAIL);
        }
        pthread_spin_unlock(&sg_MsgSpinlock);
    }
    return ret;
}
