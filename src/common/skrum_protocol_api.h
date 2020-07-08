#ifndef _SKRUM_PROTOCOL_API_H
#define _SKRUM_PROTOCOL_API_H

#include <stdio.h>

#include "src/common/skrum_protocol_defs.h"

int skrum_send_msg(int fd, skrum_msg_t *msg);
int skrum_recv_msg(int fd, skrum_msg_t *msg);

extern void skrum_setup_sockaddr(struct sockaddr_in *sin, uint16_t port);
extern int skrum_init_msg_engine_port(uint16_t selected_port, uint16_t *actual_port);

#endif
