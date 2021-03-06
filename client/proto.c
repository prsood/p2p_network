#include "proto.h"
#include <string.h>

void get_header(char *packet, header *hdr)
{
	memcpy(hdr, packet, sizeof(header));
}

void get_data(char *packet, void *data)
{
	header hdr;

	get_header(packet, &hdr);
	memcpy(data, packet + sizeof(header), hdr.size_of_data);
}
