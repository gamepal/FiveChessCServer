#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include "tcp_session.h"

#define MAX_SESSION_NUM 6000   //连接上限
#define MAX_RECV_BUFFER 8196   //缓存上限 8k

#define my_malloc malloc
#define my_free free

static struct 
{
	struct session* online_session;  //Linked List1
	struct session* session_cache;   //Linked List2
	struct session* end_ptr;        //Linked List2

	char receive_buffer[MAX_RECV_BUFFER];
	int already_readed;

	/* 0:二进制  size+content
   1:文本 用/r/n 作为结束符号 */
	int protocol_mode;
	int has_removed_session;

}session_manager;


static struct session* session_alloc()
{
	struct session* temp_sesssion = NULL;

	if (session_manager.end_ptr != NULL)
	{
		temp_sesssion = session_manager.end_ptr;
		session_manager.end_ptr = session_manager.end_ptr->next;
	}
	else  //连接超过 MAX_SESSION_NUM 调用系统分配
	{
		temp_sesssion = my_malloc(sizeof(struct session));
	}
	
	memset(temp_sesssion, 0, sizeof(struct session));
	return temp_sesssion;
}

static void session_free(struct session *s)
{
	if (s >= session_manager.session_cache
		&& s < session_manager.session_cache + MAX_SESSION_NUM)
	{
		s->next = session_manager.end_ptr;
		session_manager.end_ptr = s;
	}
	else
	{
		my_free(s);
	}
}

void init_session_manager(int bin_or_text)
{
	memset(&session_manager, 0, sizeof(session_manager));
	session_manager.protocol_mode = bin_or_text;

	session_manager.session_cache = my_malloc(MAX_SESSION_NUM * sizeof(struct session));
	memset(session_manager.session_cache, 0, MAX_SESSION_NUM * sizeof(struct session));

	for (int i = 0; i < MAX_SESSION_NUM; i++)
	{
		session_manager.session_cache[i].next = session_manager.end_ptr;
		session_manager.end_ptr = &session_manager.session_cache[i];
	}
}

void exit_session_manager()
{

}

struct session* add_session(int to_client_socket, char* ip, int port)
{
	struct session* s = session_alloc();
	s->to_client_socket = to_client_socket;
	s->to_client_port = port;
	
	int len = strlen(ip);  //192.168.0.1
	if (len >= 32)
	{
		len = 31;
	}

	strncpy(s->to_client_ip, ip, len);
	s->to_client_ip[len] = 0;

	s->next = session_manager.online_session;
	session_manager.online_session = s;

	return s;
}

void foreach_online_session(int(*callback)(struct session *s, void *p), void *p)
{
	if (callback == NULL)
	{
		return;
	}

	struct session *walk = session_manager.online_session;

	while (walk)
	{
		if (walk->is_removed == 1)
		{
			walk = walk->next;
			continue;
		}

		if (callback(walk, p))  //返回为零继续往下走 ,否则找到session需要break 
		{
			return;
		}

		walk = walk->next;
	}
}

void close_session(struct session *s)
{
	s->is_removed = 1;
	session_manager.has_removed_session = 1;
	printf("client %s:%d exit\n", s->to_client_ip, s->to_client_port);
}

static void text_process_package(struct session *s)
{

}

static void bin_process_package(struct session* s)
{
	if (session_manager.already_readed < 4)  //前4个byte为包长度(head+body).length
	{
		return;
	}

	//二进制拆包
	int *pack_head = (int*)session_manager.receive_buffer;
	int pack_len = (*pack_head);   //(head+body).length

	if (pack_len > MAX_RECV_BUFFER)
	{
		goto pack_failed;
	}

	//处理包内容
	int handle_total = 0;  //已经处理的包
	while (session_manager.already_readed >= pack_len)
	{
		//当前包收完

#pragma region 移动到下一个包
		handle_total += pack_len;

		if (session_manager.already_readed - handle_total < 4)  //head包没收完
		{
			if (session_manager.already_readed > handle_total)
			{
				//收到的包前移-复制内存内容,到dest所指的地址上,dest source num
				memmove(session_manager.receive_buffer,
					session_manager.receive_buffer + handle_total,
					session_manager.already_readed - handle_total);
			}
			session_manager.already_readed -= handle_total;
			return;
		}

		//二进制拆包
		pack_head = (int*)(handle_total + session_manager.receive_buffer);
		pack_len = (*pack_head);   //head+body
		if (pack_len > MAX_RECV_BUFFER)
		{
			goto pack_failed;
		}

		if ((session_manager.already_readed - handle_total) < pack_len)  //body包没收完
		{
			//收到的包前移-复制内存内容,到dest所指的地址上。
			memmove(session_manager.receive_buffer,
				session_manager.receive_buffer + handle_total,
				session_manager.already_readed - handle_total);

			session_manager.already_readed -= handle_total;
			return;
		}
#pragma endregion
	}

	return;
pack_failed:
	close_session(s);
}


void session_data_receivesession_data_receive(struct session *s)
{
	int readed = recv(s->to_client_socket,
		session_manager.receive_buffer + session_manager.already_readed,
		MAX_RECV_BUFFER - session_manager.already_readed, 0);

	if (readed <= 0)  //对端关闭 Tcp协议，正常断开的话，服务端Receive会返回0，
	{
		close_session(s);
		return;
	}

	session_manager.already_readed += readed;

	if (session_manager.protocol_mode == 0)
	{
		bin_process_package(s);
	}
	else
	{
		text_process_package(s);
	}

}

void clear_closed_session()
{
	if (session_manager.has_removed_session = 0)
	{
		return;
	}

	struct session **walk = &session_manager.online_session;

	while (*walk)
	{
		struct session *s = (*walk);
		if (s->is_removed)
		{
			*walk = s->next;
			s->next = NULL;
			closesocket(s->to_client_socket);
			s->to_client_socket = 0;
			session_free(s);
		}
		else
		{
			walk = &(*walk)->next;
		}
	}

	session_manager.has_removed_session = 0;
}


/*
方式1 文本协议json xml ...
方式2 二进制协议
*/