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
#include "exchange_data.h"
#include "crc.h"
#include "cJSON.h"
#define CLIENT_CONFIG "json_client.txt"
#define MAXLINE 80
#define SERV_PORT 8001
UDTSOCKET UDTSocket;
int sockfd; //全局变量，socket的文件描述符
/***************************************************************************
 * 功能：从json中获取数据端口
 * 返回：0有值，非0没有值
 ***************************************************************************/
int get_info(char *addr,unsigned int &port,char *name,char *pw)
{
	int ret=0;
	FILE *f=fopen(CLIENT_CONFIG,"rb");fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=(char*)malloc(len+1);fread(data,1,len,f);fclose(f);
	//printf("data:%s",data);
	cJSON *json=cJSON_Parse(data);
	free(data);
	//printf("d1 %s\n",cJSON_Print(json));
	int port1=cJSON_GetObjectItem(json,"serverport")->valueint;
	char *p=cJSON_GetObjectItem(json,"serverip")->valuestring;
	memcpy(addr, p, (20<=strlen(p))?20:strlen(p) );

	p=cJSON_GetObjectItem(json,"username")->valuestring;
	memcpy(name, p, (20<=strlen(p))?20:strlen(p) );

	p=cJSON_GetObjectItem(json,"passwd")->valuestring;
	memcpy(pw, p, (20<=strlen(p))?20:strlen(p) );
	//printf("port: %d\n",port1);
	port=port1;
	cJSON_Delete(json);
	return -1;
}
pthread_t ntid;
char addr[20]={0};
unsigned int port;
char name[20]={0};
char pw[20]={0};
struct sockaddr_in servaddr,servaddr1;
void *thr_send_heartbeat(void *arg);
int main(int argc, char *argv[])
{

	int  n;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	socklen_t servaddr_len;


	get_info(addr,port,name,pw);
	printf("addr:%s, port:%d\n",addr,port);
	printf("name: %s,pw: %s\n\n",name,pw);
	//return 0;



	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<=0)
	{
		perr_exit("socket:");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, addr, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	//设置接收超时
	struct timeval tv_out;
	tv_out.tv_sec = 5;//等待5秒
	tv_out.tv_usec = 0;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));


	//创建检测线程
	int err = pthread_create(&ntid, NULL, thr_send_heartbeat, NULL);
	if (err != 0) {
			fprintf(stderr, "can't create thread: %s\n", strerror(err));
			exit(1);
	}




	char name1[20]={0};
	while(1)
	{
		printf("input the name: ");
		scanf("%s",name1);
		printf("you input: %s\n",name1);
		/*构造查询数据包*/
		M_query_online mq ;
		memset(&mq, 0, sizeof(mq));
		memcpy(mq.M_QUERY_ONLINE_HEAD, M_QUERY_ONLINE_HEAD, 4);
		sprintf(mq.username_me ,name);
		sprintf(mq.username_he, name1);
		//构造crc
		mq.crc=0;
		int crc=crc32((unsigned char*)&mq, sizeof(M_heartbeat));
		mq.crc=crc;
		printf("sizeof: %d %d ", sizeof(mq),sizeof(mq));



		//查询包
		n = sendto(sockfd, &mq, sizeof(mq), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if (n == -1)
			perr_exit("sendto error");
		//sleep(1);
		memset(&mq,0,sizeof(mq));
		n = recvfrom(sockfd, &mq, sizeof(mq), 0, NULL, 0);
		if (n == -1)
			perr_exit("recvfrom error");
		unsigned int tmp=mq.crc;
		mq.crc = 0;
		printf("crc:%#X\n",tmp);
		if(tmp==crc32((unsigned char*)&mq, sizeof(mq)))
		{
			printf("got it\n");
			printf("ip:%#X ,port:%#X\n",mq.addr,mq.port);
			char ip[20];
			inet_ntop(AF_INET, &mq.addr, ip, sizeof(ip));
			printf("ip:%s,port:%d\n",ip,ntohs(mq.port));
			printf("recv ok\n");

		}
		else
		{
			printf("recv error!\n\n");
		}
	}
	Close(sockfd);
	return 0;

}


void *thr_send_heartbeat(void *arg)
{
	int n;





	while(1)
	{
		printf("start\n");
		/*构造心跳数据包*/
		M_heartbeat mh ;
		memset(&mh, 0, sizeof(mh));
		memcpy(mh.M_HEADRTBEAT_HEAD, M_HEARTBEAT_HEAD,4);
		sprintf(mh.username,name);
		sprintf(mh.passwd, pw);
		mh.tmp1=0;
		mh.tmp2=0;
		//构造crc
		mh.crc=0;
		unsigned int crc=crc32((unsigned char*)&mh, sizeof(M_heartbeat));
		mh.crc=crc;
		printf("crc1:%#X\n",crc);
		printf("crc1:%#X\n",crc32((unsigned char*)&mh, sizeof(M_heartbeat)));
		//发送
		n = sendto(sockfd, &mh, sizeof(mh), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if (n == -1)
			perr_exit("sendto error");




		//接收
		memset(&mh,0,sizeof(mh));
		n = recvfrom(sockfd, &mh, sizeof(mh), 0, NULL, 0);
		if (n == -1)
			perr_exit("recvfrom error");
		unsigned int tmp=mh.crc;
		mh.crc = 0;
		printf("crc:%#X\n",tmp);
		if(tmp==crc32((unsigned char*)&mh, sizeof(mh)))
		{
			printf("got it\n");
			printf("recv ok\n");
		}
		else
		{
			printf("recv error!\n\n");
		}
		sleep(10);
	}
}
