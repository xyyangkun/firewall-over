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
#include "exchange_data.h"
#include "Checkclientalive.h"
#include "crc.h"
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
//检查客户端的生存时间，如果没时间了就删除之。
void *thr_check_client_alive(void *arg);
int main(void)
{
	int err;
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


#if 1
	Check_client_alive cca;
	//创建检测线程
	err = pthread_create(&ntid, NULL, thr_check_client_alive, &cca);
	if (err != 0) {
			fprintf(stderr, "can't create thread: %s\n", strerror(err));
			exit(1);
	}
	//cca.add_user("yangkun");
/*	while(1)
	{
		printf("size: %d\n",cca.size());
		sleep(1);
		printf("debug!!\n");
	}*/
	//return 0;
#endif


	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	Bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));













	M_heartbeat mh ;
	memset(&mh, 0, sizeof(mh));
	memcpy(mh.M_HEADRTBEAT_HEAD, M_HEARTBEAT_HEAD,4);
	sprintf(mh.username,"yangkun");
	sprintf(mh.passwd, "123456");
	mh.tmp1=0;
	mh.tmp2=0;
	mh.crc=0;
	unsigned int crc=crc32((unsigned char *)&mh, sizeof(M_heartbeat));
	printf("crc1:%#X\n",crc);
	mh.crc=crc;


	unsigned char buf[100];
	cliaddr_len = sizeof(cliaddr);
	printf("Accepting connections ...\n");
	while (1) {
		memset(buf, 0, sizeof(buf));
		/*   收客户端地址　和　数据包头*/
		n = recvfrom(sockfd, buf, sizeof(M_heartbeat), 0, \
				(struct sockaddr *)&cliaddr, &cliaddr_len);
		if (n == -1)
		{
			perr_exit("recvfrom error");
			continue;
		}

		//打印对方发送数据包的客户端 ip 和　port
		printf("received from %s at PORT %d\n",\
		       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
		       ntohs(cliaddr.sin_port));

		//判断数据头的类型
		if( 0 == memcmp(&buf, M_HEARTBEAT_HEAD, 4))
		{
			/*心跳*/
			M_heartbeat *mhb=(M_heartbeat *)buf;
			printf("username：%s, passwd: %s\n",mhb->username,mhb->passwd);
			//1、校验crc
			unsigned int crc_tmp=mhb->crc;
			mhb->crc=0;
			unsigned int crc=crc32((unsigned char *)buf, sizeof(M_heartbeat));
			if(crc!=crc_tmp)
			{
				printf(" crc error,getcrc:%#X, recv crc:%#X\n",\
						crc,crc_tmp);
				continue;
			}
			//2、检查用户名密码是不是正确，如果正确，把这个用户加入队列里面，同时返回数据
			char pw[20]={0};
			if(0!=get_user(mhb->username,pw))
			{
				printf("username is not in");
				exit(1);
			}
			printf("pw: %s\n",pw);
			if( 0!= memcmp(pw, mhb->passwd, 20))
			{
				//密码不正确，就不返回了
				printf("passwd is wrong!!\n");
				continue;
			}
			else
			{
				printf("size: %d\n",cca.size());
				//密码正确
				//查看这个用户是不是在队列中
				if( 0!=cca.is_in(mhb->username) )
				{
					printf("new user: %s\n",mhb->username);
					cca.add_user(mhb->username,(int)cliaddr.sin_addr.s_addr,(int)cliaddr.sin_port);
				}
				else
				{
					printf("the user %s is in\n",mhb->username);
					cca.add_user_time(mhb->username,(int)cliaddr.sin_addr.s_addr,(int)cliaddr.sin_port);
				}
				printf("debug!!!\n");
				unsigned int addr,port;
				cca.get_user_info(mhb->username,addr,port);
				printf("the add: %s, port:%d \n",\
						inet_ntop(AF_INET, &addr, str, sizeof(str)),
					       ntohs(port));
				//把密码位置0，准备返回数据
				memset(mhb->passwd, 0, 20);
				crc=crc32((unsigned char *)buf, sizeof(M_heartbeat));
				mhb->crc=crc;
				n = sendto(sockfd, mhb, sizeof(M_heartbeat), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
				if (n == -1)
					perr_exit("sendto error");
				printf("send ok\n");
			}
		}
		else if( 0 == memcmp(&buf, M_QUERY_ONLINE_HEAD, 4))
		{
			/*查询另一个客户端是不是在线*/
			M_query_online *mqo=(M_query_online *)buf;
			printf("username_me：%s,username_you：%s \n",mqo->username_me,mqo->username_he);
			//1、校验crc
			unsigned int crc=mqo->crc;
			mqo->crc=0;
			if(crc!=crc32((unsigned char *)buf, sizeof(M_heartbeat)))
			{
				printf(" crc error:\n");
				continue;
			}
			//2、判断其来的ip和端口是不是正确，如果正确,返回数据
			unsigned int addr,port;
			err=cca.get_user_info(mqo->username_me,addr,port);
			if(err<0)
			{
				printf("1can't find the user:%s\n",mqo->username_me);
			}
			if(addr != cliaddr.sin_addr.s_addr )
			{
				printf("query addr error!! you from: %#X, you saved: %#X\n",\
						cliaddr.sin_addr.s_addr,addr);
				continue;
			}
			if( port != cliaddr.sin_port )
			{
				printf("query port error!!\n");
				continue;
			}
			//填充查询的信息
			err=cca.get_user_info(mqo->username_he,mqo->addr,mqo->port);
			if(err<0)
			{
				printf("2can't find the user:%s\n",mqo->username_he);
				mqo->addr=0;
				mqo->port=0;
			}
			crc=crc32((unsigned char *)buf, sizeof(M_query_online));
			mqo->crc=crc;
			n = sendto(sockfd, mqo, sizeof(M_query_online), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
			if (n == -1)
				perr_exit("sendto error");
			printf("send ok\n");
		}
		else
		{
			/*错误处理*/
			printf("error head: %#X %#X %#X %#X \n",buf[0], buf[1], buf[2], buf[3]);
		}



	}
}


void *thr_check_client_alive(void *arg)
{
	Check_client_alive *cca=static_cast< Check_client_alive * >(arg);
	cca->check();
}
