#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "socket_io/tcp_listener.h"
#include "socket_io/tcp_iocp.h"

int main(int argc, char** agv)
{
	//start_tcp_listener(6000);
	start_server(6000);  //iocp model
	return 0;
}