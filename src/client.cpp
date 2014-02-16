/* client.c */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "wrap.h"

#include "udt.h"

#define MAXLINE 80
#define SERV_PORT 8001
UDTSOCKET UDTSocket;
int sockfd; //全局变量，socket的文件描述符
void *readfile(void *ptr)
{
	printf("d33\n");

	char rbuf[100];

	int  n;
	//收
	while(1)
	{
		n = UDT::recvmsg(UDTSocket, rbuf, 100);
		if (n == -1)
		perr_exit("recvfrom error2");
		printf("buf=%s\n",rbuf);
		//Write(STDOUT_FILENO, rbuf, n);
		memset(rbuf,0,sizeof(rbuf));
	}
	return NULL;
}
int main(int argc, char *argv[])
{
	struct sockaddr_in servaddr, cliaddr;
	int n;

	//char str[INET_ADDRSTRLEN];
	//socklen_t servaddr_len;
	struct b{
		int num;
		in_addr_t ip;
		int port;
	}a;

	int num;
	if(argc != 2)  {
		fprintf(stderr,"Usage:%s  num",argv[0]);
		exit(1);
	}
	if((num = atoi(argv[1])) < 0) {
		fprintf(stderr,"Usage:%s  num",argv[0]);
		exit(1);
	}
	a.num=num;
	a.port=5555;
	a.ip=111111;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, "5.175.169.88", &servaddr.sin_addr);
	servaddr.sin_port = htons(SERV_PORT);
//发送自己的号让ser区分    目前只有1号 2号
		n = sendto(sockfd, (char *)&a, sizeof(struct b), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if (n == -1)
			perr_exit("sendto error");
		else printf("sendto1 ok\n,a.num=%d,a.port=%d,a.ip=%d\n",a.num,a.port,a.ip) ;
		memset(&a,0,sizeof(struct b));
//目前只有两个接收到另一个的端口
		int t11=sizeof(servaddr);
	n = recvfrom(sockfd, (char *)&a, sizeof(struct b), 0, (struct sockaddr *)&servaddr,(socklen_t*)&t11);
		if (n == -1)
			perr_exit("recvfrom error1");
		printf("debug:n=%d,a.port=%d,a.ip=%d\n",n,a.port,a.ip);

//改变目标的ip和端口
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(a.port);
	memcpy(&cliaddr.sin_addr,&a.ip,4);

	//debug
	struct in_addr tmp;
	tmp.s_addr=a.ip;
	printf("debug:: ip=%s,port=%d\n,num=%d",inet_ntoa(tmp),cliaddr.sin_port,a.num);



	UDTSocket=UDT::socket(AF_INET, SOCK_DGRAM, 0);
	int nOptValue = 1;
	if ( UDT::setsockopt (UDTSocket, SOL_SOCKET,UDT_RENDEZVOUS,(char*)&nOptValue , sizeof(int) ) !=0)
	{
		printf("setsockopt SO_REUSEADDR 失败%s \n",UDT::getlasterror().getErrorMessage());

	}
	if (UDT::ERROR == UDT::bind2(UDTSocket, sockfd))
	{
		printf("UDT绑定失败:%s " , UDT::getlasterror().getErrorMessage());
	}
	if (UDT::ERROR == UDT::connect(UDTSocket, (const struct sockaddr *)&cliaddr,sizeof(cliaddr)))
	{
		printf("UDT连接失败:%s\n ",UDT::getlasterror().getErrorMessage());
	}



	//接收线程
	pthread_t tid_rec;
	pthread_create(&tid_rec, NULL, readfile, (void *)&cliaddr);
		printf("recv pthread ok\n");
	//发送线程
	char buf[MAXLINE]={0};
	while (fgets(buf, MAXLINE, stdin) != NULL) {
		n= UDT::sendmsg(UDTSocket, buf, strlen(buf));
		if (n == -1)
			perr_exit("sendto error");
		else Write(STDOUT_FILENO, buf, n);
		memset(buf, 0, MAXLINE);
	}

	Close(sockfd);
	return 0;
}
