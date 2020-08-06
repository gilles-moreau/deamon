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
	MCAST_CONTROLLER_INFO,
	REQUEST_NODE_REGISTRATION,
	RESPONSE_NODE_REGISTRATION,
	RESPONSE_PONG,
} skrum_msg_type_t;

/* API configuration struct */

typedef struct skrum_msg {
	int conn_fd;              /* do not pack */
	uint16_t msg_type;
	struct sockaddr_in orig_addr;
	void *data;
	uint32_t data_size;
} skrum_msg_t;

typedef struct controller_info_msg {
	uint16_t ctrlr_port;
	time_t   ctrlr_startup_ts; 
} controller_info_msg_t;

typedef struct deamon_pong_msg {
	int16_t my_id;
} deamon_pong_msg_t;

typedef struct request_register {
	uint16_t my_port;          /* deamon rpc listening port     */
} request_register_msg_t;

typedef struct response_register {
	int16_t my_id;
} response_register_msg_t;

extern void skrum_msg_t_init(skrum_msg_t *msg);

#endif
