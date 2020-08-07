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

extern skrumd_conf_t *conf;

static int _rpc_ping(skrum_msg_t *msg);
static int _rpc_xcpuinfo(skrum_msg_t *msg);
static int _resp_register(skrum_msg_t *msg);
static int _process_ctrlr_mcast (skrum_msg_t *msg);

void skrumd_req(skrum_msg_t *msg)
{
	switch(msg->msg_type) {
		case REQUEST_PING: 
			_rpc_ping(msg);
			break;
		case REQUEST_XCPUINFO:
			_rpc_xcpuinfo(msg);
			break;
		case MCAST_CONTROLLER_INFO:
			_process_ctrlr_mcast(msg);
			break;
		default:
			error("skrumd_req: invalid request msg type %d",
					msg->msg_type);
			break;
	}
	return;
}

static int _process_ctrlr_mcast (skrum_msg_t *msg)
{
	controller_info_msg_t *dmsg;

	/* set controller data */
	dmsg = (controller_info_msg_t *)msg->data;
	conf->controller_ip = msg->orig_addr;
	conf->cont_port = dmsg->ctrlr_port;

	if (conf->ctrlr_ts != dmsg->ctrlr_startup_ts) {
		/* init request registration message */
		skrum_msg_t req_register_msg;
		skrum_msg_t resp_register_msg;
		request_register_msg_t req_reg_msg; 
		response_register_msg_t *resp_reg_msg;

		skrum_msg_t_init(&req_register_msg);

		req_register_msg.msg_type = REQUEST_NODE_REGISTRATION;
		req_reg_msg.my_port = conf->port;
		req_register_msg.data = &req_reg_msg;

		/* set new controller startup timestamp */
		conf->ctrlr_ts = dmsg->ctrlr_startup_ts;

		/* send registration request */
		if (skrum_send_recv_msg(conf->controller_ip, 
					&req_register_msg, &resp_register_msg) < 0)
			error("send register error");
		resp_reg_msg = (response_register_msg_t *)resp_register_msg.data;

		/* set node id */
		conf->cluster_id = resp_reg_msg->my_id;
		info("sent new registration. cluster node id: %d", conf->cluster_id);
	} else {
		int fd;
		skrum_msg_t response_pong;
		deamon_pong_msg_t resp_pong;

		skrum_msg_t_init(&response_pong);
		response_pong.msg_type = RESPONSE_PONG;
		resp_pong.my_id = conf->cluster_id;
		response_pong.data = &resp_pong;

		if ((fd = skrum_open_msg_conn(&conf->controller_ip)) < 0)
			return -1;

		if (skrum_send_msg(fd, &response_pong) < 0)
			return -1;
	} 

	return 0;
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
