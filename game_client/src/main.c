#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib, "WSOCK32.LIB")
#endif

int main(int argc, char** argv)
{
#ifdef WIN32
	WORD mVersionRequested;
	WSADATA wsaData;
	int err;
	mVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(mVersionRequested, &wsaData);
	if (err != 0)
	{
		return -1;
	}
#endif

	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
	{
		return -1;
	}

	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr("47.103.41.186");
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(6000);

	int ret = connect(s, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

	if (ret != 0)
	{
		printf("Connect error\n");
		closesocket(s);
		system("pause");
		goto out;
		return -1;
	}

	printf("Connect success\n");
	system("pause");
	closesocket(s);

out:
#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}