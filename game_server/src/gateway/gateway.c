#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "socket/session.h"
#include "gateway.h"


#define MAX_SERVICES 512

struct {
	struct service_module* services[MAX_SERVICES];
}gateway;

void
init_server_gateway(int socket_type, int protocal_type) {
	memset(&gateway, 0, sizeof(gateway));
	init_session_manager(socket_type, protocal_type);
}

void
exit_server_gateway() {
	exit_session_manager();
}

void
register_service(int stype, struct service_module* module) {
	if (stype <= 0 || stype >= MAX_SERVICES) {
		// 打印error
		return;
	}

	gateway.services[stype] = module;
	if (module->init_service_module) {
		module->init_service_module(module);
	}
}


void
on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	data[len] = 0;
	json_t* root = NULL;
	int ret = json_parse_document(&root, data);
	if (ret != JSON_OK || root == NULL) { // 不是一个正常的json包;
		return;
	}

	// 获取这个命令服务类型，假定0(key)为服务器类型；
	json_t* server_type = json_find_first_label(root, "0");
	server_type = server_type->child;

	if (server_type == NULL || server_type->type != JSON_NUMBER) {
		goto ended;
	}

	int stype = atoi(server_type->text); // 获取了我们的服务号。
	if (gateway.services[stype] && gateway.services[stype]->on_json_protocal_recv) {
		int ret = gateway.services[stype]->on_json_protocal_recv(gateway.services[stype]->module_data,
															     s, root, data, len);
		if (ret < 0) {
			close_session(s);
		}
	}
	// end 
ended:
	json_free_value(&root);
}

//前面4个字节为服务类型
void
on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	int stype = ((data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));

	if (gateway.services[stype] && gateway.services[stype]->on_bin_protocal_recv) 
	{
		int ret = gateway.services[stype]->on_bin_protocal_recv(gateway.services[stype]->module_data,
			s, data, len);
		if (ret < 0) {
			close_session(s);
		}
	}
}

//广播所有服务session掉线
void
on_connect_lost_entry(struct session* s) {
	for (int i = 0; i < MAX_SERVICES; i++) {
		if (gateway.services[i] && gateway.services[i]->on_connect_lost) {
			gateway.services[i]->on_connect_lost(gateway.services[i]->module_data, s);
		}
	}
}
