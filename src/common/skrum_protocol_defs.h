#ifndef _SKRUM_PROTOCOL_DEFS_H
#define _SKRUM_PROTOCOL_DEFS_H

#include <sys/socket.h>
#include <ctype.h>
#include <arpa/inet.h>

/* Skrum message type */

typedef enum {
	INTS_MSG,
	REQUEST_PING,
	REQUEST_XCPUINFO,
	MCAST_DISCOVERY,
	REQUEST_REGISTER,
	RESPONSE_REGISTER,
} skrum_msg_type_t;

/* API configuration struct */

typedef struct skrum_msg {
	int conn_fd;
	uint16_t msg_type;
	struct sockaddr_in orig_addr;
	void *data;
	uint32_t data_size;
} skrum_msg_t;

typedef struct ints {
	uint32_t n_int32;
	uint16_t n_int16;
} ints_msg_t;

typedef struct discovery {
	uint16_t controller_port;
} discovery_msg_t;

typedef struct req_register {
	uint16_t my_port;
} req_register_msg_t;

typedef struct resp_register {
	uint16_t my_id;
} resp_register_msg_t;

extern void skrum_msg_t_init(skrum_msg_t *msg);

#endif
