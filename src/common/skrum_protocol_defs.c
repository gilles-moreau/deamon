#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/skrum_protocol_defs.h"
#include "src/common/logs.h"

extern void skrum_msg_t_init(skrum_msg_t *msg) 
{ 
	memset(msg, 0, sizeof(skrum_msg_t));
	msg->msg_type = 0;
}

extern int skrum_free_msg_data(skrum_msg_type_t type, void *data)
{
	int rc = 0;
	if (!data)
		return rc;

	switch(type) {
		case MCAST_CONTROLLER_INFO:
			skrum_free_controller_info_msg(data);
			break;
		case REQUEST_NODE_REGISTRATION:
			skrum_free_request_register_msg(data);
			break;
		case RESPONSE_NODE_REGISTRATION:
			skrum_free_response_register_msg(data);
			break;
		case RESPONSE_PONG:
			skrum_free_deamon_pong_msg(data);
			break;
		default:
			error("invalid message type to be free: %d", type);
			rc = 1;
			break;	       
	}
	return rc;
}

extern void skrum_free_controller_info_msg(controller_info_msg_t *msg)
{
	free(msg);
}
	
extern void skrum_free_request_register_msg(request_register_msg_t *msg)
{
	free(msg);
}

extern void skrum_free_response_register_msg(response_register_msg_t *msg)
{
	free(msg);
}

extern void skrum_free_deamon_pong_msg(deamon_pong_msg_t *msg)
{
	free(msg);
}
