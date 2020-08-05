#ifndef _SKRUM_PROTOCOL_DEFS_H
#define _SKRUM_PROTOCOL_DEFS_H

#include <sys/socket.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>

/* Skrum message type */

typedef enum {
	REQUEST_PING,
	REQUEST_XCPUINFO,
	MCAST_DISCOVERY,
	REQUEST_NODE_REGISTRATION,
	RESPONSE_NODE_REGISTRATION,
} skrum_msg_type_t;

/* API configuration struct */

typedef struct skrum_msg {
	int conn_fd;
	uint16_t msg_type;
	struct sockaddr_in orig_addr;
	void *data;
	uint32_t data_size;
} skrum_msg_t;

typedef struct discovery {
	uint16_t controller_port;
} discovery_msg_t;

typedef struct req_register {
	uint16_t my_port;          /* rpc listening port     */
	uint16_t my_id;            /* -1 if not registered   */
} req_register_msg_t;

typedef struct resp_register {
	uint16_t my_id;
} resp_register_msg_t;

extern void skrum_msg_t_init(skrum_msg_t *msg);

#endif
