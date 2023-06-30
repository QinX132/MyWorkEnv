#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include "include.h"

#define MY_TEST_MAX_EVENTS          1024 

static int sg_ServerListenFd[MY_TEST_MAX_CLIENT_NUM_PER_SERVER + 1] = {0};
static int sg_MsgId = 0;

int CreateServerFd(
    void
    )
{
    int serverFd = -1;
	serverFd = socket(AF_INET, SOCK_STREAM, 0);     //create socket

    int32_t reuseable = 1; // set port reuseable when fd closed
    (void)setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseable, sizeof(reuseable));    // set reuseable

    int nonBlock = fcntl(serverFd, F_GETFL, 0);
    nonBlock |= O_NONBLOCK;
    fcntl(serverFd, F_SETFL, nonBlock);         // set fd nonBlock

	struct sockaddr_in localAddr = {0};
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(MY_TEST_PORT);
	localAddr.sin_addr.s_addr=htonl(INADDR_ANY);
 
	if(0 > bind(serverFd, (void *)&localAddr, sizeof(localAddr)))
	{
		printf("Bind failed\n");
		goto CommonReturn;
	}
	printf("Bind serverFd: %d\n", serverFd);

	if(0 > listen(serverFd, MY_TEST_MAX_CLIENT_NUM_PER_SERVER))
	{
		printf("Listen failed\n");
		goto CommonReturn;
	}

CommonReturn:
    return serverFd;
}

int HandleMsgs(
    int Fd,
    MY_TEST_MSG_HEAD MsgHead,
    MY_TEST_MSG_CONT *MsgCont
    )
{
    int ret = 0;
    MY_TEST_MSG *replyMsg = NULL;
    int replyLen = sizeof(MY_TEST_MSG) + MY_TEST_MSX_CONTENT_LEN;
    int currentLen = 0;
    int sendRet = 0;
    UNUSED(MsgHead);

    replyMsg = (MY_TEST_MSG *)malloc(replyLen);

    replyMsg->Head.Id = sg_MsgId ++;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    replyMsg->Head.TimeStamp = tv.tv_sec + tv.tv_usec / 1000;
    replyMsg->Head.MsgContentLen = sizeof(MY_TEST_MSG_CONT) + MY_TEST_MSX_CONTENT_LEN;

    replyMsg->Cont.MagicVer = MsgCont->MagicVer;
    replyMsg->Cont.SessionId = MsgCont->SessionId;
    memcpy(replyMsg->Cont.VarLenCont, "reply", strlen("reply"));

    for(; currentLen < replyLen;)
    {
        sendRet = send(Fd, ((char*)replyMsg) + currentLen, replyLen - currentLen, 0);
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
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            printf("%d:%s\n",errno,strerror(errno));
            goto CommonReturn;
        }
    }

CommonReturn:
    if (MsgCont)
        free(MsgCont);
    if (replyMsg)
        free(replyMsg);
    return ret;
}

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
    
    HandleMsgs(Fd, msgHead, msgCont);

    if (strcasecmp(msgCont->VarLenCont, MY_TEST_DISCONNECT_STRING) == 0)
    {
        ret = -EHOSTDOWN;
        goto CommonReturn;
    }

CommonReturn:
    return ret;
}

int main(void)
{
    int ret = 0;
	int serverFd = -1;

    serverFd = CreateServerFd();
	if (0 > serverFd)
	{
		printf("Create server socket failed\n");
		goto CommonReturn;
	}

    int epoll_fd = -1;
    int event_count = 0;
    struct epoll_event event, waitEvents[MY_TEST_MAX_EVENTS];

    epoll_fd = epoll_create1(0);
	if (0 > epoll_fd)
	{
		printf("Create epoll socket failed %d\n", errno);
		goto CommonReturn;
	}
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serverFd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverFd, &event))
    {
		printf("Add to epoll socket failed %d\n", errno);
		goto CommonReturn;
    }

    int i = 0;
    /* recv */
    while (1)
    {
        event_count = epoll_wait(epoll_fd, waitEvents, MY_TEST_MAX_EVENTS, 0);
        for(i = 0; i < event_count; i++)
        {
            if (waitEvents[i].data.fd == serverFd)
            {
                int tmpClientFd = -1;
                struct sockaddr tmpClientaddr;
                socklen_t tmpClientLen;
                tmpClientFd = accept(serverFd, &tmpClientaddr, &tmpClientLen);
                if (tmpClientFd != -1)
                {
                    int flags = fcntl(tmpClientFd, F_GETFL, 0);
                    fcntl(tmpClientFd, F_SETFL, flags | O_NONBLOCK);    // set non block

                    event.data.fd = tmpClientFd;
                    event.events = EPOLLIN | EPOLLET;  
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tmpClientFd, &event);
                    sg_ServerListenFd[0] ++;
                    sg_ServerListenFd[sg_ServerListenFd[0]] = tmpClientFd;
                }
            }
            else if (waitEvents[i].events & EPOLLIN)
            {
                ret = RecvMsgs(waitEvents[i].data.fd);
                if (ret)
                    printf("\tRecv in %d failed %d\n", waitEvents[i].data.fd, ret);
            }
            else if (waitEvents[i].events & EPOLLOUT)
            {
                printf("\tSending data to %d\n", waitEvents[i].data.fd);
            }
        }
    }

CommonReturn:
    for (i = 0; i < sg_ServerListenFd[0] ; i++)
    {
        if (sg_ServerListenFd[i+1] && sg_ServerListenFd[i+1] != -1)
            close(sg_ServerListenFd[i+1]);
    }
    if (serverFd != -1)
	    close(serverFd);
    if (epoll_fd != -1)
	    close(epoll_fd);
	return 0;
}
