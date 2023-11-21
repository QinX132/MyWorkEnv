#include "include.h"
#include "myCommonUtil.h"
#include "myModuleCommon.h"
#include "ServerMonitor.h"
#include "myClientServerMsgs.h"

#define MY_SERVER_COMM_REPLY                                "success"

typedef struct _SERVER_EXEC_CMD_ARG
{
    int Fd;
    MY_MSG *Msg;
}
SERVER_EXEC_CMD_ARG;

static void
_ServerMsgExecCmdFn(
    void *Arg
    )
{
    SERVER_EXEC_CMD_ARG *cmdArg = (SERVER_EXEC_CMD_ARG*)Arg;
    int ret = MY_SUCCESS;
    FILE *pipe = NULL;
    char buffer[MY_BUFF_1024] = {0};
    char tmpCmd[MY_BUFF_1024] = {0};

    if (!cmdArg || !cmdArg->Msg)
    {
        ret = -MY_EIO;
        LogErr("Null input ptr!");
        goto CommonReturn;
    }
    (void)snprintf(tmpCmd, sizeof(tmpCmd), "%s", (char*)cmdArg->Msg->Cont.VarLenCont);
    
    pipe = popen((char*)cmdArg->Msg->Cont.VarLenCont, "r");
    if (pipe == NULL) 
    {
        ret = -MY_EIO;
        LogErr("popen failed!");
        goto CommonReturn;
    }
    ClearMsgCont(cmdArg->Msg);
    cmdArg->Msg->Head.Type = MY_MSG_TYPE_EXEC_CMD_REPLY;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
    {
        ret = FillMsgCont(cmdArg->Msg, buffer, strlen(buffer));
        if (ret)
        {
            LogDbg("Too long Msg, break now!");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }

    if (0 == cmdArg->Msg->Head.ContentLen)
    {
        if (FillMsgCont(cmdArg->Msg, "Exec cmd: ", strlen("Exec cmd: ")) ||
            FillMsgCont(cmdArg->Msg, tmpCmd, strlen(tmpCmd)) ||
            FillMsgCont(cmdArg->Msg, " failed!", strlen(" failed!")))
        {
            LogErr("Too long Msg!");
            goto CommonReturn;
        }
    }

    ret = SendMsg(cmdArg->Fd, cmdArg->Msg);
    if (ret)
    {
        ret = -MY_EIO;
        LogErr("Send msg failed!");
        goto CommonReturn;
    }

CommonReturn:
    if (pipe)
        pclose(pipe);
    if (cmdArg)
    {
        if (cmdArg->Msg)
            FreeMsg(cmdArg->Msg);
        MyFree(cmdArg);
    }
    return ;
}

static int
_ServerMsgExecCmd(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;
    SERVER_EXEC_CMD_ARG *arg = NULL;
    MY_MSG *msg = NULL;

    arg = (SERVER_EXEC_CMD_ARG *)MyCalloc(sizeof(SERVER_EXEC_CMD_ARG));
    msg = NewMsg();
    if (LIKELY(arg && msg && Msg))
    {
        memcpy(msg, Msg, sizeof(MY_MSG));
        arg->Msg = msg;
        arg->Fd = Fd;
        ret = TPoolAddTask(_ServerMsgExecCmdFn, arg);
    }
    else
    {
        ret = -MY_ENOMEM;
    }

    if (ret)
    {
        if (msg)
            FreeMsg(msg);
        if (arg)
            MyFree(arg);
    }
    return ret;
}

static int
_ServerDefaultMsgHandler(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;
    MY_MSG *replyMsg = NewMsg();
    
    if (!replyMsg)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    replyMsg->Head.Type = Msg->Head.Type + 1;

    if (FillMsgCont(replyMsg, Msg->Cont.VarLenCont, Msg->Head.ContentLen - 1) ||
        FillMsgCont(replyMsg, (void*)":", sizeof(char)) ||
        FillMsgCont(replyMsg, (void*)MY_SERVER_COMM_REPLY, sizeof(MY_SERVER_COMM_REPLY))
        )
    {
        ret = -MY_EIO;
        LogErr("Fill msg failed");
        goto CommonReturn;
    }
    ret = SendMsg(Fd, replyMsg);
    if (ret)
    {
        LogErr("Send msg failed, ret %d:%s", ret, My_StrErr(ret));
        goto CommonReturn;
    }
CommonReturn:
    if (replyMsg)
    {
        FreeMsg(replyMsg);
    }
    return ret;
}

int 
ServerHandleMsg(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;

    if (!Msg)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }

    switch (Msg->Head.Type)
    {
        case MY_MSG_TYPE_EXEC_CMD:
            ret = _ServerMsgExecCmd(Fd, Msg);
            break;
        default:
            ret = _ServerDefaultMsgHandler(Fd, Msg);
            break;
    }

CommonReturn:
    return ret;
}

