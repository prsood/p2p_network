#ifndef DEFINE_H
#define DEFINE_H

#include <stdint.h>

/* Element in routing table */
#pragma pack(push, 1)
typedef struct
{
	uint32_t ip;
	uint16_t port;
	uint32_t time;
} client_node;
#pragma pack(pop)


/* DONT FORGET: add size of data */
#pragma pack(push, 1)
typedef struct
{
	char 	command;
	short 	size_of_data;
} header;
#pragma pack(pop)

/* Protocol properties */

void get_header(char *packet, header *hdr);
void get_data(char *packet, void *data);

#define PACKET_SIZE 1024

/* Command list */

#define PUSH 	0x01
#define REMV 	0x02
#define PULL	0x03

#endif
