/*
 * Checkclientalive.h
 *
 *  Created on: 2014-2-18
 *      Author: yangkun
 */

#ifndef CHECKCLIENTALIVE_H_
#define CHECKCLIENTALIVE_H_
#include "exchange_data.h"
#include <pthread.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <string.h>
using namespace std;
using std::vector;
//客户端的生存时间
struct Client_alive
{
	char client_name[NAME_LENGTH+1];
	unsigned int alive_time;
	unsigned int addr;			//对方的ip
	unsigned int port;			//对方的端口，用低2个字节
};
class calive
{
private:
	char client_name[NAME_LENGTH+1];
public:
	calive(char *username)
	{
		memset(client_name, 0, sizeof(client_name));
		if(!username || strlen(username) > NAME_LENGTH)
		{
			cout << "error username !!!" << endl;
			throw -1;
		}
		memcpy(client_name, username, strlen(username));
	};
	bool operator() (const struct Client_alive& c) const
	{
		//cout << "debug: c.client_name: " << c.client_name \
				<<" client_name " << client_name << endl;
		return (0==memcmp(c.client_name, client_name ,strlen(client_name)));
	}
};
class Check_client_alive
{
public:
	Check_client_alive();
	virtual ~Check_client_alive();
	int add_user_time(char *username,int addr,int port);
	int add_user(char *username,unsigned int addr,unsigned int port);
	int is_in(char *username);
	int get_user_info(char *username, unsigned int &addr,unsigned int &port);
	void check();
	unsigned int size(){return ca.size();}
private:
	pthread_mutex_t counter_mutex;
	vector<struct Client_alive> ca;

	//void will_remov(vector<struct Client_alive>::iterator iter){iter->alive_time--;if(iter->alive_time==0)ca.erase(iter);}
};

#endif /* CHECKCLIENTALIVE_H_ */
