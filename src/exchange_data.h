/*
 * exchange_data.h
 *
 *  Created on: 2014-2-16
 *      Author: xy
 */

#ifndef EXCHANGE_DATA_H_
#define EXCHANGE_DATA_H_
/***********************************************************************************
 * 此文件中所有结构的大小都为56!!!
 *
 * 1、客户端向服务器通信(带crc)：
 * 		a、M_heartbeat	心跳，可以让服务器获取自己的ip和端口
 * 		b、知道对方用户名，查询对方是否在线发送数据；服务器返回数据
 *
 * 2、客户端与客户端通信(有udt不带crc)：
 * 		a、知道用户名，且查到对方在线后，与对方联系
 *
 *
 *
 ***********************************************************************************/
#define EXCHANGE_DATA_LENGTH 56
#define NAME_LENGTH  	20			//用户名最长20个字节
#define PASSWD_LENGTH	20			//  密码最长20个字节
const char M_HEARTBEAT_HEAD[4]		=	{0xaa, 0x55, 0xa5, 0x5a};
const char M_QUERY_ONLINE_HEAD[4]	=	{0Xaa, 0x55, 0x5a, 0xa5};
const char M_SAY_HELLO_HEAD[4]		=	{0Xaa, 0x55, 0x5a, 0x5a};
//心跳包,每10秒发送一次，如果10秒不发送，服务器把这个客户端踢了
//客户端向服务器发送:填充username,passwd为自己的的。
//服务向向窗户端返回:填充username为"server",填充passwd为全0
typedef struct
{
	char M_HEADRTBEAT_HEAD[4];	//这种类型的数据的数据头
	char username[NAME_LENGTH];
	char passwd[PASSWD_LENGTH];
	unsigned int tmp1;			//无用，主要是对齐
	unsigned int tmp2;			//无用，主要是对齐
	unsigned int crc;			//这段数据的crc值
}M_heartbeat;
//通过用户名查询其是否在线
//客户端向服务器发送:addr与port任意(不过最好置0)
//服务端向客户端返回:addr与port分别是对方的,用udt去连就好了；如果没查到addr置0，port置0
typedef struct
{
	char M_QUERY_ONLINE_HEAD[4];		//这种类型的数据的数据头
	char username_me[NAME_LENGTH];		//自已的用户名
	char username_he[NAME_LENGTH];		//要查询的用户名
	unsigned int addr;			//对方的ip
	unsigned int port;			//对方的端口，用低2个字节
	unsigned int crc;			//这段数据的crc值
}M_query_online;
typedef enum
{
	start	= (unsigned char) 0,
	busy	= (unsigned char)1,
	idle	= (unsigned char)2
}M_client_state ;


//客户端与客户端通信(知道用户名，且查到对方在线后，与对方联系)
typedef struct
{
	char M_SAY_HELLO_HEAD[4];		//这种类型的数据的数据头
	char username_me[NAME_LENGTH];		//自已的用户名
	char username_you[NAME_LENGTH];		//对方的用户名
	unsigned char state;
	char say_hello[7];				//hello
	unsigned int crc;			//这段数据的crc值
}M_say_hello ;

#endif /* EXCHANGE_DATA_H_ */
