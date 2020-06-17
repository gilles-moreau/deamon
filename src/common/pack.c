/* Skrum un/pack messages */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "pack.h"

Buf create_buf (char *data, uint32_t size)
{
	Buf my_buf;
	
	if (size > MAX_BUF_SIZE) {
		error("MAX_BUF_SIZE exceed");
		return NULL;
	}

	my_buf = malloc(sizeof(struct skrum_buf));
	my_buf->size = size;
	my_buf->processed = 0;
	my_buf->head = malloc(size);

	if (!my_buf->head) {
		error("in malloc");
		return NULL;
	}
	memcpy(my_buf->head, data, size);

	return my_buf;
}

void free_buf (Buf my_buf)
{
	if (!my_buf)
		return;

	free(my_buf->head);

	free(my_buf);
}

Buf init_buf (uint32_t size)
{
	Buf my_buf;

	if (size > MAX_BUF_SIZE) {
		error("MAX_BUF_SIZE exceeded");
		return NULL;
	}

	my_buf = malloc(sizeof(struct skrum_buf));
	my_buf->size = size;
	my_buf->processed = 0;
	my_buf->head = malloc(size);

	if (!my_buf->head) {
		error("in malloc");
		return NULL;
	}

	memset(my_buf->head, 0, size);

	return my_buf;
}

uint32_t remaining_buf (Buf my_buf)
{
	return my_buf->size - my_buf->processed;
}

void pack16 (uint16_t val, Buf buffer)
{

	uint16_t ns = htons(val);
	
	if (remaining_buf(buffer) < sizeof(ns)) {
		error("BUF_SIZE exceeded");
		return;
	}

	memcpy(&buffer->head[buffer->processed], &ns, sizeof(ns));
	buffer->processed += sizeof(ns);
}

void unpack16 (uint16_t *val, Buf buffer)
{

	uint16_t ns;
	
	if (remaining_buf(buffer) < sizeof(ns)) {
		error("BUF_SIZE exceeded");
		return;
	}

	memcpy(&ns, &buffer->head[buffer->processed], sizeof(ns));
	*val = ntohs(ns);
	buffer->processed += sizeof(ns);
}

void pack32 (uint32_t val, Buf buffer)
{

	uint32_t nl = htonl(val);
	
	if (remaining_buf(buffer) < sizeof(nl)) {
		error("BUF_SIZE exceeded");
		return;
	}

	memcpy(&buffer->head[buffer->processed], &nl, sizeof(nl));
	buffer->processed += sizeof(nl);
}

void unpack32 (uint32_t *val, Buf buffer)
{

	uint32_t nl;
	
	if (remaining_buf(buffer) < sizeof(nl)) {
		error("BUF_SIZE exceeded");
		return;
	}

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));
	*val = ntohl(nl);
	buffer->processed += sizeof(nl);
}

void pack_sockaddr(struct sockaddr_in *addr, Buf buffer)
{
	pack16(addr->sin_addr.s_addr, buffer);
	pack32(addr->sin_port, buffer);
}

void unpack_sockaddr(struct sockaddr_in *addr, Buf buffer)
{

	addr->sin_family = AF_INET;
	unpack32(&addr->sin_addr.s_addr, buffer);
	unpack16(&addr->sin_port, buffer);

	addr->sin_addr.s_addr = addr->sin_addr.s_addr;
	addr->sin_port = addr->sin_port;
}
