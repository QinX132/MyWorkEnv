#include "include.h"
#include "myCommonUtil.h"
#include "myModuleCommon.h"
#include "myClientServerMsgs.h"
#include "ClientProc.h"

#define CLIENT_MSG_HANDLE_USAGE_OPT_PRINT_OFFSET                                32
#define CLIENT_MSG_HANDLE_DISCONNECT_STRING                                     "disconnect"

typedef struct _CLIENT_MSG_HANDLE
{
    char* Opt;
    char* Help;
    int32_t Argc;
    int32_t MsgType;
}
CLIENT_MSG_HANDLE;

static CLIENT_MSG_HANDLE sg_ClientMsgHandle[] = {
        {"Help", "show this page", 1, MY_MSG_TYPE_START_UNSPEC},
        {"ExecCmd", "<Cmd> call server to exec this cmd and get reply", 2, MY_MSG_TYPE_EXEC_CMD},
    };

void
ClientCmdUsage(
    void
    )
{
    size_t loop = 0;
    printf("----------------------------------------------");
    printf("----------------------------------------------\n");
    printf("%s Usage:\n\n", MY_CLIENT_ROLE_NAME);
    for (loop = 0; loop < sizeof(sg_ClientMsgHandle) / sizeof(CLIENT_MSG_HANDLE); loop ++)
        printf("%*s:[%-s]\n", CLIENT_MSG_HANDLE_USAGE_OPT_PRINT_OFFSET, sg_ClientMsgHandle[loop].Opt, 
                            sg_ClientMsgHandle[loop].Help);
    printf("\n----------------------------------------------");
    printf("----------------------------------------------\n");
}

static void 
_ClientGetArgFromCmd(
    __in char* Cmd,
    __in size_t CmdLen,
    __in char* FirstCmd,
    __in size_t FirstCmdLen,
    __in char* SecondCmd,
    __in size_t SecondCmdLen,
    __out int* Argc
    )
{
    size_t readIndex = 0, firstCmdWriteIndex = 0, secondCmdWriteIndex = 0;
    
    while(isspace(Cmd[readIndex]))  readIndex ++;
    while(readIndex < CmdLen && !isspace(Cmd[readIndex]) && firstCmdWriteIndex < FirstCmdLen - 1)
        FirstCmd[firstCmdWriteIndex ++] = Cmd[readIndex ++];
    FirstCmd[firstCmdWriteIndex] = '\0';

    while(isspace(Cmd[readIndex]))  readIndex ++;
    while(readIndex < CmdLen && secondCmdWriteIndex < SecondCmdLen - 1)
        SecondCmd[secondCmdWriteIndex ++] = Cmd[readIndex ++];
    SecondCmd[secondCmdWriteIndex] = '\0';

    if (strnlen(SecondCmd, SecondCmdLen) != 0)
    {
        *Argc = 2;
    }
    else
    {
        *Argc = 1;
    }
}

BOOL
ClientCmdSupported(
    __in char* Cmd,
    __in size_t CmdLen,
    __out int32_t* MsgType,
    __out char* MsgCont,
    __out size_t MsgContLen
    )
{
    BOOL support = FALSE;
    size_t loop = 0;
    char firstCmd[MY_BUFF_32] = {0};        // opt
    char secondCmd[MY_BUFF_128] = {0};      // content
    size_t secondCmdLen = 0;
    int argc = 0;

    if (!Cmd || !MsgType)
    {
        goto CommonReturn;
    }
    
    _ClientGetArgFromCmd(Cmd, CmdLen, firstCmd, sizeof(firstCmd), secondCmd, sizeof(secondCmd), &argc);
    
    for (loop = 0; loop < sizeof(sg_ClientMsgHandle) / sizeof(CLIENT_MSG_HANDLE); loop ++)
    {
        if (strcasecmp(firstCmd, sg_ClientMsgHandle[loop].Opt) == 0 && argc == sg_ClientMsgHandle[loop].Argc &&
            sg_ClientMsgHandle[loop].MsgType != MY_MSG_TYPE_START_UNSPEC)
        {
            support = TRUE;
            *MsgType = sg_ClientMsgHandle[loop].MsgType;
            break;
        }
    }

    if (support)
    {
        secondCmdLen = strnlen(secondCmd, sizeof(secondCmd));
        if (MsgContLen <= secondCmdLen)
        {
            support = FALSE;
        }
        else
        {
            memcpy(MsgCont, secondCmd, secondCmdLen);
        }
    }
    
CommonReturn:
    return support;
}

BOOL
ClientCmdIsExit(
    char* Cmd
    )
{
    BOOL isExit = FALSE;

    if (!Cmd)
    {
        isExit = FALSE;
        // do nothing
    }
    else if (strcasecmp(Cmd, CLIENT_MSG_HANDLE_DISCONNECT_STRING) == 0)
    {
        isExit = TRUE;
    }
    
    return isExit;
}
    
int32_t 
ClientHandleCmd(
    int32_t Fd,
    int32_t MsgType,
    char* MsgCont,
    size_t MsgContLen
    )
{
    int ret = MY_SUCCESS;
    MY_MSG *msgToSend = NULL;
    static uint32_t sessionId = 0;
    
    msgToSend = NewMsg();
    if (!msgToSend)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }

    msgToSend->Head.SessionId = sessionId ++;
    msgToSend->Head.Type = MsgType;
    switch(msgToSend->Head.Type)
    {
        case MY_MSG_TYPE_EXEC_CMD:
            ret = FillMsgCont(msgToSend, MsgCont, MsgContLen);
            if (ret)
            {
                LogErr("Fill msg failed!");
                goto CommonReturn;
            }
            break;
        default:
            ret = -MY_ENOSYS;
            LogErr("Invalid msg type %d!", msgToSend->Head.Type);
            goto CommonReturn;
    }
    
    ret = SendMsg(Fd, msgToSend);
    if (ret)
    {
        LogErr("Send msg failed!");
        goto CommonReturn;
    }
    
    FreeMsg(msgToSend);
    msgToSend = NULL;
    
CommonReturn:
    if (msgToSend)
        FreeMsg(msgToSend);
    return ret;
}

int 
ClientHandleMsg(
    int Fd,
    MY_MSG *Msg
    )
{
    int ret = MY_SUCCESS;
    
    UNUSED(Fd);
    if (Msg)
    {
        printf("<<<< Recv msg: \n%s\n", Msg->Cont.VarLenCont);
    }

    return ret;
}

