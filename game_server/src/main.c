#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "socket_io/tcp_listener.h"
//#include "socket_io/tcp_iocp.h"
#include "gateway/gateway.h"
#include "services/service_type.h"
#include "services/table_service.h"

int main(int argc, char** agv)
{
	init_server_gateway(WEB_SOCKET_IO, JSON_PROTOCAL);
	// 注册服务的模块到gateway;
	register_service(FIVE_CHESS_SERVICE, &SERVICE_TABLE);
	//// end

	start_server("0.0.0.0", 6081);
	exit_server_gateway();
	return 0;
}