#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "include.h"
 
#define PORT 13231

int main(int argc,char *argv[])
{
	int iSocketFD = 0; //socket句柄
	unsigned int iRemoteAddr = 0;
	struct sockaddr_in stRemoteAddr = {0}; //对端，即目标地址信息
	socklen_t socklen = 0;  	
	char buf[4096] = {0}; //存储接收到的数据
 
	iSocketFD = socket(AF_INET, SOCK_STREAM, 0); //建立socket
	if(0 > iSocketFD)
	{
		printf("创建socket失败！\n");
		return 0;
	}
    int r = 0;
    time_t timep;
    struct tm *p;
 
	stRemoteAddr.sin_family = AF_INET;
	stRemoteAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, argv[1], &iRemoteAddr);
	stRemoteAddr.sin_addr.s_addr=iRemoteAddr;
	
	//连接方法： 传入句柄，目标地址，和大小
	if(0 > connect(iSocketFD, (void *)&stRemoteAddr, sizeof(stRemoteAddr)))
	{
		printf("连接失败！\n");
		//printf("connect failed:%d",errno);//失败时也可打印errno
	}else{
		printf("连接成功！\n");
        while (1){
            memset(buf,0,sizeof(buf));
            r = recv(iSocketFD, buf, sizeof(buf), 0);
            if (r == 0)
                continue;
            else if (r != -1)
            {
                time(&timep);
                p = gmtime(&timep);
                printf("%d-%d-%d %d:%d:%d\tRecv> %s\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec ,buf);
            }
            else
            {
                time(&timep);
                p = gmtime(&timep);
                printf("%d-%d-%d %d:%d:%d\terror> %d:%s\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec, errno, strerror(errno));
            }
        }
	}
	
	close(iSocketFD);//关闭socket	
	return 0;
}
