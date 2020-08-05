#ifndef _SKRUM_PROTOCOL_API_H
#define _SKRUM_PROTOCOL_API_H

#include <stdio.h>

#include "src/common/skrum_protocol_defs.h"

int skrum_send_msg(int fd, skrum_msg_t *msg);
int skrum_recv_msg(int fd, skrum_msg_t *msg);
extern int skrum_send_msg(int fd, skrum_msg_t *msg);
extern int skrum_receive_msg(int fd, skrum_msg_t *msg);
extern int skrum_open_msg_conn(struct sockaddr_in *addr);
extern int skrum_send_recv_msg(struct sockaddr_in dest_addr, 
		skrum_msg_t *request_msg, skrum_msg_t *response_msg);

extern int skrum_init_msg_engine_port(uint16_t selected_port, uint16_t *actual_port);

#endif
