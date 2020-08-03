/* Skrumd request handler */

#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_defs.h"

#include "src/skrumd/skrumd.h"

#include "src/skrumd/skrumd_req.h"

static int _rpc_ping(skrum_msg_t *msg);
static int _rpc_xcpuinfo(skrum_msg_t *msg);
static int _resp_register(skrum_msg_t *msg);

void skrumd_req(skrum_msg_t *msg)
{
	switch(msg->msg_type) {
		case REQUEST_PING: 
			_rpc_ping(msg);
			break;
		case REQUEST_XCPUINFO:
			_rpc_xcpuinfo(msg);
			break;
		case RESPONSE_REGISTER:
			_resp_register(msg);
			break;
		default:
			error("skrumd_req: invalid request msg type %d",
					msg->msg_type);
			break;
	}
	return;
}

static int _resp_register(skrum_msg_t *msg)
{
	int rc = 0;
	int conn_fd = msg->conn_fd;
	skrum_msg_t resp_msg;
	resp_register_msg_t resp_register;

	resp_msg.msg_type = 
	resp_register.my_id = 1;
		
	return rc;
}

static int _rpc_xcpuinfo(skrum_msg_t *msg)
{
	int rc = 0;

	return rc;
}

static int _rpc_ping(skrum_msg_t *msg)
{
	int rc = 0;

	return rc;

}
