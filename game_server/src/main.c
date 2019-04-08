#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "socket_io/tcp_listener.h"

int main(int argc, char** agv)
{
	start_tcp_listener(6000);
	printf("Server start\n");
	return 0;
}