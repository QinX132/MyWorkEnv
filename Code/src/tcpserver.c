#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include "include.h"

#define BACKLOG 5	    //最大监听数
#define PORT 13231

int main(int argc,char *argv[])
{
	int32_t keepAlive = 1; //open TCP keepalive
   	int32_t keepIdle = 90; //if this TCP connection has no data in 90s, then begin to detect
    int32_t keepInterval = 3; // TCP detect interval is 3s
    int32_t keepCount = 10; // TCP detect timeouts is 10

	int iSocketFD = 0;  //socket句柄
	int iRecvLen = 0;   //接收成功后的返回值
	int new_fd = 0; 	//建立连接后的句柄
	char buf[4096] = {0}; //
	struct sockaddr_in stLocalAddr = {0}; //本地地址信息结构图，下面有具体的属性赋值
	struct sockaddr_in stRemoteAddr = {0}; //对方地址信息
	socklen_t socklen = 0;
    
    int r = 0;
    time_t timep;
    struct tm *p;
 
	iSocketFD = socket(AF_INET, SOCK_STREAM, 0); //建立socket
	if(0 > iSocketFD)
	{
		printf("创建socket失败！\n");
		return 0;
	}
	
    (void)setsockopt(iSocketFD, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
    (void)setsockopt(iSocketFD, SOL_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
    (void)setsockopt(iSocketFD, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
    (void)setsockopt(iSocketFD, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));

	stLocalAddr.sin_family = AF_INET;  /*该属性表示接收本机或其他机器传输*/
	stLocalAddr.sin_port = htons(PORT); /*端口号*/
	stLocalAddr.sin_addr.s_addr=htonl(INADDR_ANY); /*IP，括号内容表示本机IP*/
 
	//绑定地址结构体和socket
	if(0 > bind(iSocketFD, (void *)&stLocalAddr, sizeof(stLocalAddr)))
	{
		printf("绑定失败！\n");
		return 0;
	}
 
	//开启监听 ，第二个参数是最大监听数
	if(0 > listen(iSocketFD, BACKLOG))
	{
		printf("监听失败！\n");
		return 0;
	}
 
	printf("iSocketFD: %d\n", iSocketFD);	
	//在这里阻塞知道接收到消息，参数分别是socket句柄，接收到的地址信息以及大小 
	new_fd = accept(iSocketFD, (void *)&stRemoteAddr, &socklen);
	if(0 > new_fd)
	{
		printf("接收失败！\n");
		return 0;
	}else{
		printf("接收成功！\n");
		//发送内容，参数分别是连接句柄，内容，大小，其他信息（设为0即可） 
        while (1){
            r = send(new_fd, "Ack", strlen("ACK"), 0);
            if (r == 0)
                continue;
            else if (r != -1)
            {
                time(&timep);
                p = gmtime(&timep);
                printf("%d-%d-%d %d:%d:%d\tSend> Ack\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec );
            }
            else
            {
                time(&timep);
                p = gmtime(&timep);
                printf("%d-%d-%d %d:%d:%d\terror> %d:%s\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, 8 + p->tm_hour, p->tm_min, p->tm_sec , errno, strerror(errno));
            }
            sleep(2);
        }
	}
 
	close(new_fd);
	close(iSocketFD);
 
	return 0;
}
