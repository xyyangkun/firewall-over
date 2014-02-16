/* server.c */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "wrap.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_t ntid;
#define SERV_PORT 8001
struct b{
		int num;
		int ip;
		int port;
	};
struct b a,a1,a2;

struct sockaddr_in servaddr, cliaddr;
struct sockaddr_in b1,b2;
socklen_t cliaddr_len;
int sockfd;
char str[INET_ADDRSTRLEN];
int i, n;

void *thr_fn(void *arg)
{
printf(arg);
	while(1)
	{
		printf("111\n");
		printf(":::%d:::%d\n",a1.port,a2.port);
		sleep(1);
		if((a1.port!=6666)&&(a2.port!=6666))
		{
			printf("\t\tok\n");
			cliaddr.sin_family = AF_INET;
			cliaddr.sin_addr.s_addr = htonl(a1.ip);
			cliaddr.sin_port = htons(a1.port);
			int n = sendto(sockfd, &a2, sizeof(struct b), 0, (struct sockaddr *)&b1, sizeof(cliaddr));
			if (n == -1)
				perr_exit("sendto error");
			else printf("a2ok\n");

			cliaddr.sin_family = AF_INET;
			cliaddr.sin_addr.s_addr = htonl(a2.ip);
			cliaddr.sin_port = htons(a2.port);
			n = sendto(sockfd, &a1, sizeof(struct b), 0, (struct sockaddr *)&b2, sizeof(cliaddr));
			if (n == -1)
				perr_exit("sendto error");
			else printf("a21ok\n");
			break;
		}
	}
	return NULL;
}

int main(void)
{
	memset(&a, 0, sizeof(struct b));
	memset(&a1, 0, sizeof(struct b));
	memset(&a2, 0, sizeof(struct b));
	a1.port=6666;
	a2.port=6666;


	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
    
	Bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	int err;
	err = pthread_create(&ntid, NULL, thr_fn, "new thread: ");
	if (err != 0) {
			fprintf(stderr, "can't create thread: %s\n", strerror(err));
			exit(1);
	}
	printf("Accepting connections ...\n");
	while (1) {
		if((a1.port!=6666)&&(a2.port!=6666))continue;

		cliaddr_len = sizeof(cliaddr);
		/*   收客户端地址   */
		n = recvfrom(sockfd, (char *)&a, sizeof(struct b), 0, (struct sockaddr *)&cliaddr, &cliaddr_len);
		if (n == -1)
			perr_exit("recvfrom error");
		printf("received from %s at PORT %d\n",\
		       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
		       ntohs(cliaddr.sin_port));
		if(a.port!=5555)
			printf("protel err\n");
printf("d1\n");
		if(a.num==1)
		{
			memcpy(&b1,&cliaddr,cliaddr_len);
			a1.port=ntohs(cliaddr.sin_port);
			memcpy(&a1.ip,&cliaddr.sin_addr,4);
			printf("\t\t num=%d,ip=%s,port=%d\n",a.num,inet_ntoa(a1.ip),a1.port);
		}else
		{
			memcpy(&b2,&cliaddr,cliaddr_len);
			a2.port=ntohs(cliaddr.sin_port);
			memcpy(&a2.ip,&cliaddr.sin_addr,4);
			printf("\t\t num=%d,ip=%s,port=%d\n",a.num,inet_ntoa(a2.ip),a2.port);
		}
		a.port=6666;

	}
}
