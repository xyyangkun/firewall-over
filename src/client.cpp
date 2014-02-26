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
UDTSOCKET UDTSocket;
int sockfd; //全局变量，socket的文件描述符
pthread_t ntid_heartbeat;
pthread_t ntid_listen;
pthread_t ntid_chat;
/*从配置文件提取的信息*/
char addr[20]={0};
unsigned int port;
char name[20]={0};
char pw[20]={0};
/*从配置文件提取的信息*/
struct sockaddr_in servaddr,cliaddr;
void *thr_send_heartbeat(void *arg);
int get_info(char *addr,unsigned int &port,char *name,char *pw);
void *thr_chat_frineds(void *arg);
void *thr_listen_frineds(void *arg);
bool isrun=true;
static unsigned int send_recv_time=60; //每发一次心跳包，加一次，每收一次心跳包减一次，到0时程序退出
M_client_state mcs = idle; //客户端是空闲的

//查询后从接收线程取得信息后，填入此结构体，可能要加锁
struct query_user_info
{
	char name[20];
	int ip;
	int port;
	int socket;
}qui;
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
		mylog_log( LOG4C_PRIORITY_ERROR, "socket:");
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


	//创建心跳包
	int err = pthread_create(&ntid_heartbeat, NULL, thr_send_heartbeat, NULL);
	if (err != 0)
	{
		mylog_log( LOG4C_PRIORITY_ERROR, "can't create thread: %s\n", strerror(err));
			exit(1);
	}
	//创建建接收客户端或服务端的线程
	err = pthread_create(&ntid_listen, NULL, thr_listen_frineds,NULL);
	if (err != 0)
	{
		mylog_log( LOG4C_PRIORITY_ERROR, "can't create thread: %s\n", strerror(err));
			exit(1);
	}
	char name1[20]={0};
	int choice;
	while(1)
	{
		choice=0;
		printf("input the function: \n\t1、query user\n\t2、chat to the user!!\n");
		scanf("%d",&choice);
		printf("you input choice: %d\n",choice);
		while(mcs == busy)sleep(1);
		if(choice==1)
		{
			printf("please input the user you want query!!\n");
			scanf("%s",&name1);
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
			printf("send ok\n");
		}
		else if(choice==2)
		{
			printf("d1\n");
			if(qui.ip==0)
			{
				printf("please choice 1 first!!");
				continue;
			}
			printf("d2\n");
			M_say_hello msh;
			memcpy(msh.M_SAY_HELLO_HEAD, M_SAY_HELLO_HEAD, 4);
			memcpy(msh.username_me, name, 20);
			memcpy(msh.username_you, qui.name, 20);
			printf("name me:%s ,name you: %s",name, qui.name);
			char s[16];
			printf("ip %s port %d\n",\
					inet_ntop(AF_INET, &(qui.ip), s, sizeof(s)),
					ntohs(qui.port));
			msh.state = idle;
			msh.crc = 0;
			unsigned int crc=crc32((unsigned char *)&msh, sizeof(M_say_hello));
			msh.crc = crc;
			struct sockaddr_in  addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = qui.ip;
			addr.sin_port =qui.port;
			printf("size: %d , crc:%#X\n",sizeof(msh),msh.crc);
			int n = sendto(sockfd, &msh, sizeof(msh), 0, (struct sockaddr *)&(addr), sizeof(struct sockaddr ));
			if (n == -1)
			{
				mylog_log(LOG4C_PRIORITY_ERROR,"sendto error:%s\n",strerror(errno));
			}
			printf("send over\n");
		}
		else
			printf("you input not 1 eithor 2!\n");

		sleep(3);
		while(mcs == busy)sleep(1);
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
		mylog_log(LOG4C_PRIORITY_DEBUG,"listen_frineds start\n");
		char recvbuf[EXCHANGE_DATA_LENGTH];
		char str[INET_ADDRSTRLEN];
		//接收
		memset(&recvbuf,0,sizeof(recvbuf));
		memset(&cliaddr,0,sizeof(cliaddr));
		socklen_t cliaddr_len=sizeof(cliaddr);
		n = recvfrom(sockfd, &recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&cliaddr, &cliaddr_len);
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"recvfrom error:%s\n",strerror(errno));
		}


		//打印对方发送数据包的客户端 ip 和 port
		//printf("received from %s at PORT %d\n",\
		       inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),\
		       ntohs(cliaddr.sin_port));
		//printf("head: %#X %#X %#X %#X \n",\
							recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3]);
		//判断数据头的类型
		if( 0 == memcmp(&recvbuf, M_HEARTBEAT_HEAD, 4))
		{
			/*心跳*/
			M_heartbeat *mhb=(M_heartbeat *)recvbuf;
			mylog_log(LOG4C_PRIORITY_DEBUG,"username：%s, passwd: %s\n",mhb->username,mhb->passwd);
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
			mylog_log(LOG4C_PRIORITY_DEBUG,"heart beat return,now time is:%d ",send_recv_time);
		}
		else if( 0 == memcmp(&recvbuf, M_QUERY_ONLINE_HEAD, 4))
		{
			/*查询另一个客户端是不是在线*/
			//printf("query!!!!\n");
			M_query_online *mqo=(M_query_online *)recvbuf;
			//printf("username_me：%s,username_you：%s \n",mqo->username_me,mqo->username_he);
			//1、校验crc
			unsigned int crc=mqo->crc;
			mqo->crc=0;
			if(crc!=crc32((unsigned char *)recvbuf, sizeof(M_heartbeat)))
			{
				printf(" crc error:\n");
				continue;
			}
			//2、提取查询的用户名、ip和port,并把这些信息放入全局变量的
			memcpy(qui.name, mqo->username_he, 20);
			qui.ip = mqo->addr;
			qui.port = mqo->port;
			//３、提醒别人，我收到了查询包的返回
			printf("query bag return ok!!!\n");

		}
		else if( 0 == memcmp(&recvbuf, M_SAY_HELLO_HEAD, 4))
		{
			printf("recv a hello!!\n\n\n");
			M_say_hello *msh=(M_say_hello *)recvbuf;
			//1、校验crc
			unsigned int crc_tmp=msh->crc;
			msh->crc=0;
			unsigned int crc=crc32((unsigned char *)recvbuf, sizeof(M_say_hello));
			msh->crc=crc_tmp;
			if(crc!=crc_tmp)
			{
				printf(" crc error,getcrc:%#X, recv crc:%#X\n",\
						crc,crc_tmp);
				continue;
			}
			//crc正确
			printf("me：%s, you: %s\n",msh->username_me,msh->username_you);
			//bug-fix 先不测用户名了



			//２、新建socket,如果不忙就与之与通信；如果忙就给它一个忙标志
			if(msh->state == busy && mcs == idle)
			{
				//对方忙，我不忙           这种情况是我先联系对方的，这种情况不处理
				printf("he is busy,but I am idel");
				continue;
			}
			else if(msh->state == idle && mcs == busy)
			{
				//对方不忙，我忙           这种情况是对方查到我，对方先联系我的，要回复我忙
				printf("he is idel,but I am busy");
				msh->state = busy;

				// bugfix 暂时不考虑用户名
				n = sendto(sockfd, &msh, sizeof(msh), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
				if (n == -1)
				{
					mylog_log(LOG4C_PRIORITY_ERROR,"sendto error:%s\n",strerror(errno));
				}
			}
			else if(msh->state == busy && mcs == busy)
			{
				//对方忙，我也忙   这种不会发生！！！！
				printf("he is busy,and I am busy");
				continue;
			}
			else if(msh->state == idle && mcs == idle)
			{
				//对方不忙，我也不忙         这种情况是一般的正常情况，应该把自己置成忙，同时建立新的socket给系统，以替换现在的socket
				printf("he is idle,and I am idle too!");
				mcs = busy;
				qui.socket = sockfd;
				//给对方返回
				n = sendto(sockfd, &recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&cliaddr, cliaddr_len);
				if (n == -1)
				{
					mylog_log(LOG4C_PRIORITY_ERROR,"sendto error:%s\n",strerror(errno));
				}
				//记录对方的端口
				static struct query_user_info userinfo;
				memcpy(userinfo.name, qui.name, 20);
				userinfo.socket=sockfd;
				userinfo.ip = cliaddr.sin_addr.s_addr;
				userinfo.port=cliaddr.sin_port;

				//新建一个udp的socket
				sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
				if(sockfd<=0)
				{
					mylog_log( LOG4C_PRIORITY_ERROR, "socket:");
					perr_exit("socket:");
					exit(1);
				}

				//通知创建成功，并开始聊天
				//创建客户端与客户端之间的聊天线程
				printf("create chat!!!\n\n");
				int err = pthread_create(&ntid_chat, NULL, thr_chat_frineds,(void *)&userinfo);
				if (err != 0)
				{
					mylog_log( LOG4C_PRIORITY_ERROR, "can't create thread: %s\n", strerror(err));
						exit(1);
				}
				//聊天后，设置mcs为空闲


			}

		}
		else
		{
			printf("error!!!\n");
			/*错误处理*/
			mylog_log(LOG4C_PRIORITY_ERROR,"error head: %#X %#X %#X %#X \n",\
					recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3]);
		}

	}
}


/**********************************************************************************
 * 功能：与另一个用户程序聊天的线程
 *
 *
 **********************************************************************************/
void *thr_chat_frineds(void *arg)
{
	printf("ddddddddd\n\n");
	struct query_user_info user_info;
	memcpy(&user_info, arg, sizeof(query_user_info));
	char s[16];
	printf("name: %s \n",user_info.name);
	printf("char ip %s port %d\n",\
			inet_ntop(AF_INET, &(user_info.ip), s, sizeof(s)),
			ntohs(user_info.port));
	string str;
	char buf[1500];
	while(1)
	{
		cout << "input quit you can exit!and the other message will send to your friends:" << endl;
		cin>>str;
		if(str=="quie")
		{
			mcs = idle;
			break;
		}
		// bugfix 暂时不考虑用户名
		struct sockaddr_in  addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = user_info.ip;
		addr.sin_port =user_info.port;
		int n = sendto(user_info.socket, str.c_str(), str.size(), 0, (struct sockaddr *)&(addr), sizeof(struct sockaddr ));
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"sendto error:%s\n",strerror(errno));
		}

		//接收
		socklen_t addr_len=sizeof(addr);
		memset(buf, 0, sizeof(buf));
		memset(&addr, 0, sizeof(addr));
		n = recvfrom(user_info.socket, &buf, sizeof(buf), 0, (struct sockaddr *)&addr, &addr_len);
		if (n == -1)
		{
			mylog_log(LOG4C_PRIORITY_ERROR,"recvfrom error:%s\n",strerror(errno));
		}
		printf("recv buf: %s \n",buf);
		char s[16];
		printf("11received from %s at PORT %d\n",\
				inet_ntop(AF_INET, &addr.sin_addr, s, sizeof(s)),
				ntohs(addr.sin_port));


	}
	cout << "I will exit the chat thread!!" << endl;
}
