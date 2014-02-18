/*
 * Checkclientalive.cpp
 *
 *  Created on: 2014-2-18
 *      Author: yangkun
 */

#include "Checkclientalive.h"

Check_client_alive::Check_client_alive()
{
	pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
}

Check_client_alive::~Check_client_alive()
{
	// TODO Auto-generated destructor stub
}
int Check_client_alive::add_user_time(char *username)
{
	if(!username || strlen(username) > NAME_LENGTH)
	{
		cout << "error username !!!" << endl;
		return -1;
	}
	vector<struct Client_alive>::iterator iter;
	iter = find_if(ca.begin(), ca.end(), calive(username));
	if(iter!= ca.end())
	{
		pthread_mutex_lock(&counter_mutex);
		iter->alive_time ++;
		pthread_mutex_unlock(&counter_mutex);
		return 0;
	}
	return -1;
}
int Check_client_alive::add_user(char *username)
{
	if(!username || strlen(username) > NAME_LENGTH)
	{
		cout << "error username !!!" << endl;
		return -1;
	}
	struct Client_alive c_alive;
	memcpy(c_alive.client_name, username, strlen(username));
	c_alive.alive_time = 10;	//开始定义60秒时间，如果时间为0则这个用户被删除
	pthread_mutex_lock(&counter_mutex);
	ca.push_back (c_alive);
	pthread_mutex_unlock(&counter_mutex);
	return 0;
}
int Check_client_alive::is_in(char *username)
{
	if(!username || strlen(username) > NAME_LENGTH)
	{
		cout << "error username !!!" << endl;
		return -1;
	}
	vector<struct Client_alive>::iterator iter;
	iter = find_if(ca.begin(), ca.end(), calive(username));
	if(iter!= ca.end())
	{
		return 0;
	}
	return -1;
}
void Check_client_alive::check()
{
	vector<struct Client_alive>::iterator iter;
	while(1)
	{
		sleep(1);
		if(ca.size()<=0)continue;
		for (iter=ca.begin(); iter!=ca.end(); ++iter)
		{
			if(ca.size()<=0)break;
			pthread_mutex_lock(&counter_mutex);
			iter->alive_time--;
			pthread_mutex_unlock(&counter_mutex);
			if(iter->alive_time==0)
			{
				pthread_mutex_lock(&counter_mutex);
				ca.erase(iter);
				pthread_mutex_unlock(&counter_mutex);
			}
		}
	}
}
