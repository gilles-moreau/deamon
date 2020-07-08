/* Skrumd request handler */

#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_defs.h"

#include "skrumd_req.h"

static int _rpc_ping(skrum_msg_t *msg);
static int _rpc_xcpuinfo(skrum_msg_t *msg);

void skrumd_req(skrum_msg_t *msg)
{
	switch(msg->msg_type) {
		case REQUEST_PING: 
			_rpc_ping(msg);
			break;
		case REQUEST_XCPUINFO:
			_rpc_xcpuinfo(msg);
			break;
		default:
			error("skrumd_req: invalid request msg type %d",
					msg->msg_type);
			break;
	}
	return;
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
