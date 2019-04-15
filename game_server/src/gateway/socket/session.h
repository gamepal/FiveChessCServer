#ifndef __SESSION_H__
#define __SESSION_H__

#define MAX_SEND_PKG 2048  //缓存上限 2K

struct session {
	char client_ip[32];
	int  client_port;
	int  client_socket;
	int  is_removed;
	int  is_shake_hand;

	struct session* next;
	unsigned char send_buf[MAX_SEND_PKG]; //90%发送的命令缓存
};

void init_session_manager(int client_socket, int protocal_type);
void exit_session_manager();


// 有客服端进来，保存这个session;
struct session* save_session(int client_socket, char* client_ip, int client_port);
//关闭单个sesssion
void close_session(struct session* s);

// 遍历我们session集合里面的所有session
void foreach_online_session(int(*callback)(struct session* s, void* param), void*param);

// 把下线的sesssion都关闭掉
void clear_offline_session();
// end 

int
get_socket_type();

int
get_protocal_type();
#endif