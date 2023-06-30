#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "include.h"

static uint32_t currentSessionId = 0;

int RecvMsgs(
    int Fd
    )
{
    int ret = 0;
    MY_TEST_MSG_HEAD msgHead;
    MY_TEST_MSG_CONT *msgCont = NULL;
    int recvLen = sizeof(MY_TEST_MSG_HEAD);
    int currentLen = 0;
    int recvRet = 0;

    memset(&msgHead, 0, sizeof(msgHead));
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)&msgHead) + currentLen, recvLen - currentLen, 0);
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
                printf("recv ends!\n");
                goto CommonReturn;
            }
            else
            {
                ret = -errno;
                printf("%d:%s\n",errno,strerror(errno));
                goto CommonReturn;
            }
        }
    }
    printf("Id=%u, TimeStamp=%llu, MsgContentLen=%u\n", msgHead.Id, msgHead.TimeStamp, msgHead.MsgContentLen);

    msgCont = (MY_TEST_MSG_CONT *)malloc(msgHead.MsgContentLen);
    memset(msgCont, 0, msgHead.MsgContentLen);
    recvLen = msgHead.MsgContentLen;
    currentLen = 0;
    recvRet = 0;
    for(; currentLen < recvLen;)
    {
        recvRet = recv(Fd, ((char*)msgCont) + currentLen, recvLen - currentLen, 0);
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
            printf("%d:%s\n",errno,strerror(errno));
            goto CommonReturn;
        }
    }
    printf("MagicVer=%u, SessionId=%u, VarLenCont=%s\n", msgCont->MagicVer, msgCont->SessionId, msgCont->VarLenCont);

    if (strcasecmp(msgCont->VarLenCont, MY_TEST_DISCONNECT_STRING) == 0)
    {
        ret = -EHOSTDOWN;
        goto CommonReturn;
    }

CommonReturn:
    return ret;
}

int main(
    int argc,
    char *argv[]
    )
{
	int clientFd = -1;
	unsigned int serverIp = 0;
	struct sockaddr_in serverAddr = {0};
	socklen_t socklen = 0; 
    struct timeval tv;

    if (argc != 2)
    {
        printf("Usage: ./tcpclient ip\n");
        goto CommonReturn;
    }
 
	clientFd = socket(AF_INET, SOCK_STREAM, 0);
	if(0 > clientFd)
	{
		printf("Create socket failed\n");
		goto CommonReturn;
	}
    
    int32_t reuseable = 1; // set port reuseable when fd closed
    (void)setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(MY_TEST_PORT);
	inet_pton(AF_INET, argv[1], &serverIp);
	serverAddr.sin_addr.s_addr=serverIp;
	
	if(0 > connect(clientFd, (void *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Connect failed\n");
		goto CommonReturn;
	}
    printf("Connect success\n");

    MY_TEST_MSG *msgToSend = NULL;
    /* send */
    while (1)
    {
	    char buf[4096] = {0};
        uint32_t bufLen = 0;
        scanf("%s",buf);
        uint32_t stringLen = strlen(buf) + 1;

        msgToSend = (MY_TEST_MSG *)malloc(sizeof(MY_TEST_MSG) + stringLen);
        memset(msgToSend, 0 ,sizeof(msgToSend));

        msgToSend->Head.Id = 1;
        gettimeofday(&tv, NULL);
        msgToSend->Head.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;
        msgToSend->Head.MsgContentLen = sizeof(MY_TEST_MSG_CONT) + stringLen;
        msgToSend->Cont.MagicVer = 0xff;
        msgToSend->Cont.SessionId = currentSessionId ++;
        memcpy(msgToSend->Cont.VarLenCont, buf, stringLen);

        int sendLen = sizeof(MY_TEST_MSG) + stringLen;
        int currentLen = 0;
        int sendRet = 0;
        for(; currentLen < sendLen;)
        {
            sendRet = send(clientFd, ((char*)msgToSend) + currentLen, sendLen - currentLen, 0);
            if (sendRet > 0)
            {
                currentLen += sendRet;
            }
            else if (sendRet == 0)
            {
                goto CommonReturn;
            }
            else
            {
                printf("%d:%s\n",errno,strerror(errno));
                goto CommonReturn;
            }
        }

        RecvMsgs(clientFd);

        free(msgToSend);
        msgToSend = NULL;
        if (strcasecmp(buf, MY_TEST_DISCONNECT_STRING) == 0)
        {
            break;
        }
    }

CommonReturn:
    if (clientFd != -1)
	    close(clientFd);//关闭socket
    if (msgToSend)
        free(msgToSend);
	return 0;
}
