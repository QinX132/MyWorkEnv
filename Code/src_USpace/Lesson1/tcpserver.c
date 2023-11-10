#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "include.h"

int main(void)
{
    int ret = 0;
	int serverFd = -1;
	int clientFd = -1;
	struct sockaddr_in localAddr = {0};
	struct sockaddr_in peerAddr = {0};
	char buf[4096] = {0};
	socklen_t socklen = 0;
 
	serverFd = socket(AF_INET, SOCK_STREAM, 0); //建立socket
	if(0 > serverFd)
	{
		printf("Create socked failed\n");
		goto CommonReturn;
	}
	
    int32_t reuseable = 1; // set port reuseable when fd closed
    (void)setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));

	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(MY_TEST_PORT);
	localAddr.sin_addr.s_addr=htonl(INADDR_ANY);
 
	if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
	{
		printf("Bind failed\n");
		goto CommonReturn;
	}
	printf("Bind serverFd: %d\n", serverFd);

	if(0 > listen(serverFd, 5))
	{
		printf("Listen failed\n");
		goto CommonReturn;
	}
 	
	clientFd = accept(serverFd, (void *)&peerAddr, &socklen);
	if(0 > clientFd)
	{
		printf("Accept clientFd failed\n");
		goto CommonReturn;
	}
	printf("Accept clientFd: %d\n", clientFd);

    MY_TEST_MSG_HEAD msgHead;
    MY_TEST_MSG_CONT *msgCont = NULL;
    /* recv */
    while (1)
    {
        memset(&msgHead, 0, sizeof(msgHead));

        int recvLen = sizeof(MY_TEST_MSG_HEAD);
        int currentLen = 0;
        int recvRet = 0;
        for(; currentLen < recvLen;)
        {
            recvRet = recv(clientFd, ((char*)&msgHead) + currentLen, recvLen - currentLen, 0);
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
        printf("Id=%u, TimeStamp=%llu, MsgContentLen=%u\n", msgHead.Id, msgHead.TimeStamp, msgHead.MsgContentLen);

        msgCont = (MY_TEST_MSG_CONT *)malloc(msgHead.MsgContentLen);
        memset(msgCont, 0, msgHead.MsgContentLen);
        recvLen = msgHead.MsgContentLen;
        currentLen = 0;
        recvRet = 0;
        for(; currentLen < recvLen;)
        {
            recvRet = recv(clientFd, ((char*)msgCont) + currentLen, recvLen - currentLen, 0);
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
            break;
        }
        free(msgCont);
        msgCont = NULL;
    }

CommonReturn:
    if (clientFd != -1)
	    close(clientFd);
    if (serverFd != -1)
	    close(serverFd);
    if (msgCont)
        free(msgCont);
	return 0;
}
