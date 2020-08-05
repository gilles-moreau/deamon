/* Skrum un/pack messages */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <arpa/inet.h>

#include "src/common/logs.h"

#include "src/common/pack.h"

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

uint64_t hton_uint64 (uint64_t val)
{
	int endianness = 1;
	
	if ((int)((char *)&endianness)[0] == endianness) {
		uint32_t high_part; uint32_t low_part;
		high_part = htonl((uint32_t) (val >> 32));
		low_part  = htonl((uint32_t) (val & 0xFFFFFFFF));
		return ((uint64_t)low_part) << 32 | high_part;
	} else 
		return val;
}

uint64_t ntoh_uint64 (uint64_t val) 
{
	int endianness = 1;
	
	if ((int)((char *)&endianness)[0] == endianness) {
		uint32_t high_part; uint32_t low_part;
		high_part = ntohl((uint32_t) (val & 0xFFFFFFFF));
		low_part  = ntohl((uint32_t) (val >> 32));
		return ((uint64_t)high_part) << 32 | low_part;
	} else
		return val;
}

int64_t hton_int64 (int64_t val)
{
	int endianness = 1;
	
	if ((int)((char *)&endianness)[0] == endianness) {
		int32_t high_part; int32_t low_part;
		high_part = htonl((int32_t) (val >> 32));
		low_part  = htonl((int32_t) (val & 0xFFFFFFFF));
		return ((int64_t)low_part) << 32 | high_part;
	} else 
		return val;
}

int64_t ntoh_int64 (int64_t val) 
{
	int endianness = 1;
	
	if ((int)((char *)&endianness)[0] == endianness) {
		int32_t high_part; int32_t low_part;
		high_part = ntohl((int32_t) (val & 0xFFFFFFFF));
		low_part  = ntohl((int32_t) (val >> 32));
		return ((int64_t)high_part) << 32 | low_part;
	} else
		return val;
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

void pack_sockaddr(struct sockaddr_in const *addr, Buf buffer)
{
	pack16(addr->sin_family, buffer);
	pack16(addr->sin_port, buffer);
	pack32(addr->sin_addr.s_addr, buffer);
	packmem(addr->sin_zero, 8, buffer);
}

void unpack_sockaddr(struct sockaddr_in *addr, Buf buffer)
{
	uint32_t sin_zero_size;

	unpack16(&addr->sin_family, buffer);	
	unpack16(&addr->sin_port, buffer);
	unpack32(&addr->sin_addr.s_addr, buffer);
	unpackmem(&addr->sin_zero, &sin_zero_size, buffer);
}

void packmem(char *valp, uint32_t size_val, Buf buffer)
{
	uint32_t nl = htonl(size_val);

	if (remaining_buf(buffer) < (sizeof(nl) + size_val)) {
		error("%s: BUF_SIZE exceeded", __func__);
		return;
	}

	memcpy(&buffer->head[buffer->processed], &nl, sizeof(nl));
	buffer->processed += sizeof(nl);

	if (size_val) {
		memcpy(&buffer->head[buffer->processed], valp, size_val);
		buffer->processed += size_val;
	}
}

void unpackmem(char *valp, uint32_t *size_valp, Buf buffer)
{
	uint32_t nl;

	if (remaining_buf(buffer) < sizeof(nl)) {
		error("%s, BUF_SIZE exceeded", __func__);
		return;
	}

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));
	*size_valp = ntohl(nl);
	buffer->processed += sizeof(nl);

	if (*size_valp > 0) {
		if (remaining_buf(buffer) < *size_valp) {
			error("%s, BUF_SIZE exceeded", __func__);
			return;
		}
		memcpy(valp, &buffer->head[buffer->processed], *size_valp);
		buffer->processed += *size_valp;
	} else 
		*valp = 0;
}

void pack_time(time_t val, Buf buffer)
{
	int64_t ns = hton_int64((int64_t) val);

	if (remaining_buf(buffer) < sizeof(ns)) {
		error("%s, BUF_SIZE exceeded", __func__);
		return;
	}

	memcpy(&buffer->head[buffer->processed], &ns, sizeof(ns));
	buffer->processed += sizeof(ns);
}

void unpack_time(time_t *val, Buf buffer)
{
	int64_t nl;

	if (remaining_buf(buffer) < sizeof(nl)) {
		error("%s, BUF_SIZE exceeded", __func__);
		return;
	}

	memcpy(&nl, &buffer->head[buffer->processed], sizeof(nl));
	*val = (time_t) ntoh_int64(nl);
	buffer->processed += sizeof(nl);
}
