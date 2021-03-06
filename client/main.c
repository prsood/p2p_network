#include <stdio.h>
#include "proto.h"
#include "iptable.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 666


SOCKET bind_udp_sock(uint16_t port);
DWORD WINAPI handle_udp_packets(void *sock);
DWORD WINAPI broadcast(void *arg);

void run_as_server(void);
void run_as_client(void);
void menu(void);

int main(int argc, char *argv[])
{
	WSADATA wsa;

	if (FAILED(WSAStartup(MAKEWORD(2, 2), &wsa)))
	{
		printf("Error while initializing WinSock 2 library\n");
		goto _cleanup;
	}

	menu();

_cleanup:
	WSACleanup();
	exit(0);
}

void menu(void)
{
	printf("Run as server(s) or client(c)?\n");
	printf("enter> ");

	int answer = getchar();
	switch(answer)
	{
		case 's':
			run_as_server();
			break;
		case 'c':
		default:
			run_as_client();
	}
	
}

void run_as_server(void)
{
	SOCKET udp_sock = bind_udp_sock(SERVER_PORT);

	if (udp_sock != INVALID_SOCKET)
	{
		DWORD thID = 0;
		HANDLE handler = CreateThread(NULL, 0, handle_udp_packets, &udp_sock, 0, &thID);
		WaitForSingleObject(handler, INFINITE);
	}
}

void run_as_client(void)
{
	srand(time(0));
	uint16_t port = rand() % 0xAA00 + 1024;
	SOCKET udp_sock = bind_udp_sock(port);
	client_node srv = { inet_addr(SERVER_HOST), htons(666), 0 };

	if (udp_sock != INVALID_SOCKET)
	{
		pull_peer_list(udp_sock, &srv);

		DWORD thID = 0;
		HANDLE handler = CreateThread(NULL, 0, handle_udp_packets, &udp_sock, 0, &thID);
		CreateThread(NULL, 0, broadcast, &udp_sock, 0, &thID);
		WaitForSingleObject(handler, INFINITE);
	}
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

SOCKET bind_udp_sock(uint16_t port)
{
	SOCKET sock;
	SOCKADDR_IN local_addr;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Error while creating UDP socket: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	srand(time(0));
	local_addr.sin_family 		= AF_INET;
	local_addr.sin_addr.s_addr 	= INADDR_ANY;
	local_addr.sin_port 		= htons(port);

	if (bind(sock, (struct sockaddr *) &local_addr, sizeof(local_addr)) == SOCKET_ERROR)
	{
		printf("Error while binding udp socket: %d\n", WSAGetLastError());
		return INVALID_SOCKET;
	}

	return sock;
}

void get_peer_addr(struct sockaddr_in * addr, client_node * remote)
{
	remote->ip 		= addr->sin_addr.s_addr;
	remote->port 	= addr->sin_port;
	remote->time 	= time(0);
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
			case PULL:
			{
				printf("GET_LIST command recieved\n");
				client_node peer;

				get_peer_addr(&client_addr, &peer);
				add_node(&peer);

				get_data(packet, &peer);
				show_node_data(&peer);
				push_peer_list(sock, &peer);

				show_iptable();
				break;
			}
			case PUSH:
			{
				printf("ADD_NODE command recieved\n");
				client_node peer;

				get_data(packet, &peer);
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

DWORD WINAPI broadcast(void *udp)
{
	SOCKET sock = *(SOCKET *) udp;
	while (1)
	{
		broadcast_peer_list(sock);
		printf("broadcast peer list\n");
		Sleep(5000);
	}
	return 0;
}
