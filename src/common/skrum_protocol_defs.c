#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/skrum_protocol_defs.h"

extern void skrum_msg_t_init(skrum_msg_t *msg) 
{ 
	memset(msg, 0, sizeof(skrum_msg_t));
	msg->msg_type = 0;
}

