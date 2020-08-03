/* Skrumctld request handler */

#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_defs.h"

#include "src/skrumctld/skrumctld.h"

#include "src/skrumctld/skrumctld_req.h"

static int _req_register(skrum_msg_t *msg);

void skrumctld_req(skrum_msg_t *msg)
{
	switch(msg->msg_type) {
		case REQUEST_REGISTER:
			_req_register(msg);
			break;
		default:
			error("skrumctld_req: invalid request msg type %d",
					msg->msg_type);
			break;
	}
	return;
}

static int _req_register(skrum_msg_t *msg)
{
	int rc = 0;
	req_register_msg_t *req_register = (req_register_msg_t *)msg->data;
		
	info("slave sent ping listening on port %d", req_register->my_port);

	return rc;
}

