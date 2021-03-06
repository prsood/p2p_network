#include "iptable.h"

#include <stdlib.h>
#include <string.h>


#define NODE_TABLE_SIZE 255

static const client_node null_node = { 0 };
static client_node routing_table[NODE_TABLE_SIZE] = { 0 };
static size_t node_ptr = 0;
static size_t node_count = 0;


static client_node * node_already_exists(client_node * node)
{
	for (size_t node_idx = 0; node_idx < NODE_TABLE_SIZE; ++node_idx)
	{
		if (routing_table[node_idx].ip == node->ip && routing_table[node_idx].port == node->port)
			return &routing_table[node_idx];
	}
	return NULL;
}

static int compare_time_asc(const void *x, const void *y)
{
	return ((client_node *)x)->time - ((client_node *)y)->time;
}

static int compare_time_desc(const void *x, const void *y)
{
	return ((client_node *)y)->time - ((client_node *)x)->time;
}

static void sort_by_time_asc(void)
{
	qsort(routing_table, node_count, sizeof(client_node), compare_time_asc);
}

static void sort_by_time_desc(void)
{
	qsort(routing_table, node_count, sizeof(client_node), compare_time_desc);
}

void add_node(client_node * new_node)
{
	client_node *tmp = NULL;

	if ((tmp = node_already_exists(new_node)) != NULL)
	{
		tmp->time = new_node->time;
	}
	else
	{
		routing_table[node_ptr++] = *new_node;
		node_ptr %= NODE_TABLE_SIZE;
		if (node_count < NODE_TABLE_SIZE)
			++node_count;
	}
	sort_by_time_asc();
}

void send_iptable(SOCKET sock)
{
	char packet[PACKET_SIZE] = { 0 };
	header hdr = { 0 };

	hdr.command = PULL;
	hdr.size_of_data = sizeof(client_node);
	memcpy(packet, &hdr, sizeof(header));

	for (size_t node_idx = 0; node_idx < NODE_TABLE_SIZE; ++node_idx)
	{
		if (memcmp(&routing_table[node_idx], &null_node, sizeof(client_node)) != 0)
		{
			memcpy(&packet[sizeof(header)], &routing_table[node_idx], sizeof(client_node));
			send(sock, packet, PACKET_SIZE, 0);
		}
	}
}

void send_packet(SOCKET udp, client_node *node, char command, void *data, size_t size)
{
	struct sockaddr_in dest_addr;
	char packet[PACKET_SIZE] = { 0 };

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = node->ip;
	dest_addr.sin_port = node->port;

	header *hdr = (header *) packet;
	hdr->command = command;
	hdr->size_of_data = size;

	memcpy(&packet[sizeof(header)], data, size);

	sendto(udp, packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

void push_peer_list(SOCKET udp, client_node * peer)
{
	for (size_t node_idx = 0; node_idx < NODE_TABLE_SIZE; ++node_idx)
	{
		if (memcmp(&routing_table[node_idx], &null_node, sizeof(routing_table[node_idx])) != 0)
		{
			client_node node = routing_table[node_idx];
			node.time = time(0) - node.time;

			send_packet(udp, peer, PUSH, &node, sizeof(node));
		}
	}
}

void pull_peer_list(SOCKET udp, client_node * peer)
{
	send_packet(udp, peer, PULL, NULL, 0);
}

void broadcast_peer_list(SOCKET udp)
{
	for (size_t node_idx = 0; node_idx < NODE_TABLE_SIZE; ++node_idx)
	{
		if (memcmp(&routing_table[node_idx], &null_node, sizeof(routing_table[node_idx])) != 0)
		{
			push_peer_list(udp, &routing_table[node_idx]);
		}
	}
}

void show_node_data(client_node * client)
{
	struct in_addr addr;
	addr.s_addr = client->ip;

	printf("%s\t %d\t %d\t\n", inet_ntoa(addr), client->port, client->time);
}

void show_iptable(void)
{
	system("cls");
	for (size_t node_idx = 0; node_idx < NODE_TABLE_SIZE; ++node_idx)
	{
		if (memcmp(&routing_table[node_idx], &null_node, sizeof(client_node)) != 0)
			show_node_data(&routing_table[node_idx]);
	}
}
