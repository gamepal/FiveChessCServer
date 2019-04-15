#ifndef __TCP_SESSION_H__
#define __TCP_SESSION_H__

struct session
{
	char to_client_ip[32];
	int to_client_port;
	int to_client_socket;
	int is_removed;
	int is_shake_hand;
	void *player_data;
	struct session *next;
};

void init_session_manager(int protocol_mode);

// 有客服端进来，保存这个sesssion;
struct session* add_session(int to_client_socket, char* ip, int port);

void exit_session_manager();

struct session* add_session(int to_client_socket, char *ip, int port);

void foreach_online_session(int(*callback)(struct session *s, void *p), void *p);

void session_data_receive(struct session *s);

void close_session(struct session *s);

void clear_closed_session();

#endif
