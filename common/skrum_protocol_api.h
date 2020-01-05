#ifndef _SKRUM_PROTOCOL_API_H
#define _SKRUM_PROTOCOL_API_H

#include <stdio.h>

#include "skrum_protocol_pack.h"
#include "skrum_protocol_defs.h"

int skrum_send_msg(int fd, skrum_msg_t *msg);
int skrum_recv_msg(int fd, skrum_msg_t *msg);

#endif
