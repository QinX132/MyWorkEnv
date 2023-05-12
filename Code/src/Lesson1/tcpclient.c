#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "include.h"

int main(
    int argc,
    char *argv[]
    )
{
	int clientFd = -1;
	unsigned int serverIp = 0;
	struct sockaddr_in serverAddr = {0};
	socklen_t socklen = 0;  	
 
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
        msgToSend->Head.TimeStamp = 1;
        msgToSend->Head.MsgContentLen = sizeof(MY_TEST_MSG_CONT) + stringLen;
        msgToSend->Cont.MagicVer = 0xff;
        msgToSend->Cont.SessionId = 0xff;
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
