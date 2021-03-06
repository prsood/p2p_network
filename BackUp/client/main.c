#include <stdio.h>
#include "proto.h"
#include "iptable.h"
#include "server.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 666


SOCKET bind_udp_sock(client_node *myinfo);
DWORD WINAPI handle_udp_packets(void *sock);
DWORD WINAPI refresh_iptable(void *arg);


int main(int argc, char *argv[])
{
	WSADATA wsa;
	HANDLE hMutex = NULL;
	SOCKET server;
	SOCKET udp_listener;
	client_node my_node;

	/*CreateMutex(NULL, TRUE, "muu===");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		goto _cleanup;*/

	if (FAILED(WSAStartup(MAKEWORD(2, 2), &wsa)))
	{
		printf("Error while initializing WinSock 2 library\n");
		goto _cleanup;
	}

	server 	= connect_to_server(SERVER_HOST, SERVER_PORT);
	if (server != INVALID_SOCKET)
	{
		udp_listener = bind_udp_sock(&my_node);
		register_on_server(server, &my_node);
		get_nodes_from_server(server);
		shutdown(server, 0);

		DWORD thID = 0;
		HANDLE udp_handler = CreateThread(NULL, 0, handle_udp_packets, &udp_listener, 0, &thID);
		HANDLE refresher   = CreateThread(NULL, 0, refresh_iptable, &my_node, 0, &thID);

		WaitForSingleObject(udp_handler, INFINITE);
	}

_cleanup:
	WSACleanup();
	CloseHandle(hMutex);
	exit(0);
}

static void get_local_ip(char *ip_str)
{
	char hostname[255] = { 0 };
	PHOSTENT hostinfo = NULL;
	char *ip = NULL;

	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		if ((hostinfo = gethostbyname(hostname)) != NULL)
		{
			ip = inet_ntoa(*(struct in_addr *)hostinfo->h_addr_list[0]);
			strcpy(ip_str, ip);
		}
	}
}

SOCKET bind_udp_sock(client_node *myinfo)
{
	SOCKET sock;
	SOCKADDR_IN local_addr;
	char ip[50] = { 0 };

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Error while creating UDP socket: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	srand(time(0));
	local_addr.sin_family 		= AF_INET;
	local_addr.sin_addr.s_addr 	= INADDR_ANY;
	local_addr.sin_port 		= htons(rand() % 0xAA00 + 1024);

	if (bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr)) == SOCKET_ERROR)
	{
		printf("Error while binding udp socket: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}
	get_local_ip(ip);

	myinfo->ip 		= inet_addr(ip);
	myinfo->port 	= local_addr.sin_port;
	myinfo->time 	= time(0);

	return sock;
}

/*
	Recieves packets from peers using listening udp socket.
	Performs actions declared in the 'command' field of header.
*/
DWORD WINAPI handle_udp_packets(void *listener)
{
	SOCKET sock = *(SOCKET *) listener;
	struct sockaddr_in client_addr;
	char packet[PACKET_SIZE];

	while (1)
	{
		int client_addr_size = sizeof(client_addr);
		memset(&client_addr, 0, sizeof(client_addr));
		int size = recvfrom(sock, packet, PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_size);
		if (size == SOCKET_ERROR)
		{
			printf("recvfrom() error: %d\n", WSAGetLastError());
			continue;
		}

		header *hdr = (header *) packet;
		switch(hdr->command)
		{
			case GET_LIST:
			{
				printf("GET_LIST command recieved\n");
				client_node peer;
				memcpy(&peer, &packet[sizeof(header)], sizeof(client_node));
				send_iptable_to_node_udp(&peer);
				break;
			}
			case ADD_NODE:
			{
				printf("ADD_NODE command recieved\n");
				client_node peer;
				memcpy(&peer, &packet[sizeof(header)], sizeof(client_node));
				add_node(&peer);
				show_iptable();
				break;
			}
			default:
				printf("OTHER COMMAND\n");
				break;
		}
	}
	return 0;
}

DWORD WINAPI refresh_iptable(void *mynode)
{
	while (1)
	{
		send_packet_to_all_udp(GET_LIST, mynode, sizeof(client_node));
		printf("Refresh command sent (GET LIST)\n");
		Sleep(5000);
	}
	return 0;
}
