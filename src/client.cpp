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

/******************日志*******************************/
//这个宏一定要放在头文件前面
#define MYLOG_CATEGORY_NAME "log4c.firewall-over.client_write"
#include "mylog.h"
log4c_category_t* mycat = NULL;
/******************日志*******************************/

#define CLIENT_CONFIG "json_client.txt"
#define MAXLINE 80
#define SERV_PORT 8001
UDTSOCKET UDTSocket;
int sockfd; //全局变量，socket的文件描述符

pthread_t ntid;
char addr[20]={0};
unsigned int port;
char name[20]={0};
char pw[20]={0};
struct sockaddr_in servaddr,cliaddr;
void *thr_send_heartbeat(void *arg);
int get_info(char *addr,unsigned int &port,char *name,char *pw);
bool isrun=true;
static unsigned int send_recv_time=60; //每发一次心跳包，加一次，每收一次心跳包减一次，到0时程序退出
int main(int argc, char *argv[])
{

	int  n;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	socklen_t servaddr_len;
	/******************日志初始化*******************************/
	if (mylog_init())
	{
		printf("mylog_init() failed");
		return -1;
	}
	/******************日志初始化*******************************/
	get_info(addr,port,name,pw);
	mylog_log( LOG4C_PRIORITY_DEBUG, "addr:%s, port:%d,name: %s,pw: %s\n",addr,port,name,pw);



	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd<=0)
	{
		log4c_category_log(mycat, LOG4C_PRIORITY_ERROR, "socket:");
		perr_exit("socket:");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, addr, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	//设置接收超时
	//不用这些功能了。
/*	struct timeval tv_out;
	tv_out.tv_sec = 5;//等待5秒
	tv_out.tv_usec = 0;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv_out, sizeof(tv_out));*/


	//创建检测线程
	int err = pthread_create(&ntid, NULL, thr_send_heartbeat, NULL);
	if (err != 0) {
		mylog_log( LOG4C_PRIORITY_ERROR, "can't create thread: %s\n", strerror(err));
			exit(1);
	}



/*聊天功能：	１、访问一个用户 vist
 * 			２、被一个用户访问后
 * 			３、与这个用户断开连接 quit
 *
 * 			接收到hello后,如果不忙立刻新建一个与服务器通信和端口，返回hello,并用udt　；忙了，返回一个忙标志
 *
 */
	char name1[20]={0};
	while(1)
	{
		//printf("usage: 1,query username:")
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
		mylog_log(LOG4C_PRIORITY_DEBUG,"sizeof: %d %d ", sizeof(mq),sizeof(mq));



		//查询包
		n = sendto(sockfd, &mq, sizeof(mq), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if (n == -1)
			perr_exit("sendto error");
		//sleep(1);
		memset(&mq,0,sizeof(mq));
		n = recvfrom(sockfd, &mq, sizeof(mq), 0, NULL, 0);
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"recvfrom error:%s\n",strerror(err));
		}
		unsigned int tmp=mq.crc;
		mq.crc = 0;
		//printf("crc:%#X\n",tmp);
		if(tmp==crc32((unsigned char*)&mq, sizeof(mq)))
		{
			mylog_log(LOG4C_PRIORITY_INFO,"got it\n");
			mylog_log(LOG4C_PRIORITY_INFO,"ip:%#X ,port:%#X\n",mq.addr,mq.port);
			char ip[20];
			inet_ntop(AF_INET, &mq.addr, ip, sizeof(ip));
			mylog_log(LOG4C_PRIORITY_INFO,"ip:%s,port:%d\n",ip,ntohs(mq.port));
			mylog_log(LOG4C_PRIORITY_INFO,"recv ok\n");

		}
		else
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"recv error!\n\n");
		}
	}
	//线程清理操作
	//....

	Close(sockfd);
	/******************日志清理*******************************/
	//日志清理操作
	if (mylog_fini())
	{
		printf("mylog_fini() failed");
	}
	/******************日志清理*******************************/
	return 0;

}

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
void *thr_send_heartbeat(void *arg)
{
	int n;
	while(isrun)
	{
		mylog_log(LOG4C_PRIORITY_INFO,"start\n");
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
		mylog_log(LOG4C_PRIORITY_INFO,"crc1:%#X\n",crc);
		mylog_log(LOG4C_PRIORITY_INFO,"crc1:%#X\n",crc32((unsigned char*)&mh, sizeof(M_heartbeat)));
		//发送
		n = sendto(sockfd, &mh, sizeof(mh), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"sendto error:%s\n",strerror(errno));
		}
		send_recv_time--;
		if(send_recv_time==0)
		{
			isrun=false;
			continue;
		}
		mylog_log(LOG4C_PRIORITY_INFO,"heart beat send,now time is:%d ",send_recv_time);
		sleep(10);
	}
}
/**********************************************************************************
 * 功能：等着接收服务端或者其它客户端发来的消息
 *
 *
 **********************************************************************************/
void *thr_listen_frineds(void *arg)
{
	int n;
	while(isrun)
	{
		mylog_log(LOG4C_PRIORITY_INFO,"listen_frineds start\n");
		char recvbuf[EXCHANGE_DATA_LENGTH];
		char str[INET_ADDRSTRLEN];
		//接收
		memset(&recvbuf,0,sizeof(recvbuf));
		socklen_t cliaddr_len=sizeof(cliaddr);
		n = recvfrom(sockfd, &recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&cliaddr, &cliaddr_len);
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"recvfrom error:%s\n",strerror(errno));
		}


		//打印对方发送数据包的客户端 ip 和　port
		mylog_log(LOG4C_PRIORITY_INFO,"received from %s at PORT %d\n",\
		       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
		       ntohs(cliaddr.sin_port));

		//判断数据头的类型
		if( 0 == memcmp(&recvbuf, M_HEARTBEAT_HEAD, 4))
		{
			/*心跳*/
			M_heartbeat *mhb=(M_heartbeat *)recvbuf;
			mylog_log(LOG4C_PRIORITY_INFO,"username：%s, passwd: %s\n",mhb->username,mhb->passwd);
			//1、校验crc
			unsigned int crc_tmp=mhb->crc;
			mhb->crc=0;
			unsigned int crc=crc32((unsigned char *)recvbuf, sizeof(M_heartbeat));
			if(crc!=crc_tmp)
			{
				mylog_log(LOG4C_PRIORITY_ERROR," crc error,getcrc:%#X, recv crc:%#X\n",\
						crc,crc_tmp);
				continue;
			}
			send_recv_time++;
			mylog_log(LOG4C_PRIORITY_INFO,"heart beat return,now time is:%d ",send_recv_time);
		}
		else if( 0 == memcmp(&recvbuf, M_SAY_HELLO_HEAD, 4))
		{
			M_say_hello *msh=(M_say_hello *)recvbuf;
			//1、校验crc
			unsigned int crc_tmp=msh->crc;
			msh->crc=0;
			unsigned int crc=crc32((unsigned char *)recvbuf, sizeof(M_say_hello));
			if(crc!=crc_tmp)
			{
				mylog_log(LOG4C_PRIORITY_ERROR," crc error,getcrc:%#X, recv crc:%#X\n",\
						crc,crc_tmp);
				continue;
			}
			//crc正确
			mylog_log(LOG4C_PRIORITY_INFO,"me：%s, you: %s\n",msh->username_me,msh->username_you);

			//２、新建socket,如果不忙就与之与通信；如果忙就给它一个忙标志
			//２.1 不忙,新建socket ，阻塞主线程；用udt与之通信

			//2.2　忙，返回忙标志位



			//３、与客户端通信
		}
		else
		{
			/*错误处理*/
			mylog_log(LOG4C_PRIORITY_ERROR,"error head: %#X %#X %#X %#X \n",\
					recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3]);
		}

	}
}
