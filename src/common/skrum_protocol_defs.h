#ifndef _SKRUM_PROTOCOL_DEFS_H
#define _SKRUM_PROTOCOL_DEFS_H

#include <sys/socket.h>
#include <inttypes.h>
#include <arpa/inet.h>

/* Skrum message types */

typedef enum {
	INTS_MSG,
} skrum_msg_type_t;

typedef struct skrum_msg {
	uint16_t msg_type;
	struct sockaddr_in orig_addr;
	void *data;
	uint32_t data_size;
} skrum_msg_t;

typedef struct ints {
	uint32_t n_int32;
	uint16_t n_int16;
} ints_msg_t;

#endif
