/* server.c */
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "wrap.h"
#include "cJSON.h"
//服务器配置
#define SERVER_CONFIG "json_server.txt"

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
/***************************************************************************
 * 功能：从json中由用户名获取密码
 * 返回：0有值，非0没有值
 ***************************************************************************/
int get_user(char *username,char *passwd)
{
	int ret=0;
	FILE *f=fopen(SERVER_CONFIG,"rb");fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
	//printf("data:%s",data);
	cJSON *json=cJSON_Parse(data);
	free(data);
	//printf("d1 %s\n",cJSON_Print(json));
	cJSON *userlist=cJSON_GetObjectItem(json,"userlist");
	int i;
	int size=cJSON_GetArraySize(userlist);
	for(i=0; i<size; i++)
	{
		cJSON *tmp=cJSON_GetArrayItem(userlist,i);
		char *str=cJSON_GetObjectItem(tmp,"username")->valuestring;
		//printf("username:%s,,,%s,,,%d,,,%d\n",username,str,strlen(username),strlen(str));
		if( 0 == memcmp(str,username,strlen(username) ))
		{
			char *str_tmp=cJSON_GetObjectItem(tmp,"passwd")->valuestring;
			memcpy(passwd, str_tmp, strlen(str_tmp));
			//printf("str_tmp:%s\n",str_tmp);
			cJSON_Delete(json);
			return 0;
		}
	}
	cJSON_Delete(json);
	return -1;
}
/***************************************************************************
 * 功能：从json中获取数据端口
 * 返回：0有值，非0没有值
 ***************************************************************************/
int get_port(int *port)
{
	int ret=0;
	FILE *f=fopen(SERVER_CONFIG,"rb");fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
	//printf("data:%s",data);
	cJSON *json=cJSON_Parse(data);
	free(data);
	//printf("d1 %s\n",cJSON_Print(json));
	int port1=cJSON_GetObjectItem(json,"port")->valueint;
	//printf("port: %d\n",port1);
	*port=port1;
	cJSON_Delete(json);
	return -1;
}
void *thr_fn(void *arg)
{
//printf(arg);
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
	if(access(SERVER_CONFIG,0)!=0)
	{
		printf("%s is not existen\n",SERVER_CONFIG);
		exit(1);
	}
	//printf("existen\n");
	char *username="yangkun";
	char pw[20]={0};
	if(0!=get_user(username,pw))
	{
		printf("username is not in");
		exit(1);
	}
	printf("pw: %s\n",pw);

	int port;
	get_port(&port);
	printf("port:%d\n",port);
	return 0;



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
	char *p="new thread: ";
	err = pthread_create(&ntid, NULL, thr_fn, p);
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
			printf("\t\t num=%d,ip=%s,port=%d\n",a.num,inet_ntoa( *(struct in_addr *)&(a1.ip)),a1.port);
		}else
		{
			memcpy(&b2,&cliaddr,cliaddr_len);
			a2.port=ntohs(cliaddr.sin_port);
			memcpy(&a2.ip,&cliaddr.sin_addr,4);
			printf("\t\t num=%d,ip=%s,port=%d\n",a.num,inet_ntoa(*(struct in_addr *)&(a2.ip)),a2.port);
		}
		a.port=6666;

	}
}
