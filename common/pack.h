#ifndef _PACK_H
#define _PACK_H

#include <inttypes.h>

#define BUF_SIZE (16 * 1024)
#define MAX_BUF_SIZE ((uint32_t) 0xffff0000)

typedef struct skrum_buf {
	char *head;
	uint32_t size;
	uint32_t processed;
} buf_t;

typedef struct skrum_buf *Buf;

#define get_buf_data(__buf)	(__buf->head)
#define get_buf_offset(__buf)	(__buf->processed)
#define size_buf(__buf)		(__buf->size)

Buf create_buf (char *data, uint32_t size);
void free_buf (Buf my_buf);
Buf init_buf(uint32_t size);
uint32_t remaining_bug (Buf my_buf);

void pack16 (uint16_t val, Buf buffer);
void unpack16 (uint16_t *val, Buf buffer);

void pack32 (uint32_t val, Buf buffer);
void unpack32 (uint32_t *val, Buf buffer);

#endif
