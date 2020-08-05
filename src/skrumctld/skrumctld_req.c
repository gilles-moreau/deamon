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

extern List cluster_node_list;
extern skrumctld_conf_t *conf;

static int node_cnt = 0;

static int _req_register(skrum_msg_t *msg);
static int _register_cluster_node(req_register_msg_t *req);
static int _step_nodeid_match(void *x, void *key);

void skrumctld_req(skrum_msg_t *msg)
{
	switch(msg->msg_type) {
		case REQUEST_NODE_REGISTRATION:
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
	resp_register_msg_t resp_register;
	skrum_msg_t resp_msg;
	uint16_t node_id; 
	
	node_id = _register_cluster_node(req_register);

	resp_msg.msg_type = RESPONSE_NODE_REGISTRATION;
	resp_msg.orig_addr = conf->controller_ip;
	resp_register.my_id = node_id;
	resp_msg.data = &resp_register;

	if ((rc = skrum_send_msg(msg->conn_fd, &resp_msg)) < 0)
		error("could not send resp register msg");
	
	return rc;
}

static int _register_cluster_node(req_register_msg_t *req)
{
	int node_id;
	skrum_cluster_node_t *nnode;
        
	if (req->my_id < 0) {
		/* create node in list and assign id */
		nnode = malloc(sizeof(skrum_cluster_node_t));
		nnode->cluster_node_id = ++node_cnt;
		nnode->registration_ts = time(NULL); 
		nnode->rpc_port = req->my_port;
		list_append(cluster_node_list, nnode);
	} else { 
		ListIterator node_itr = list_iterator_create(cluster_node_list);
		nnode = (skrum_cluster_node_t *)list_find(node_itr, 
				_step_nodeid_match, &req->my_id);
		nnode->registration_ts = time(NULL);

		list_iterator_destroy(node_itr);
	}

	return nnode->cluster_node_id;
}

static int _step_nodeid_match(void *x, void *key)
{
	skrum_cluster_node_t *node = (skrum_cluster_node_t *)x;		
	uint16_t *id_key = (uint16_t *)key;

	if (node->cluster_node_id == *id_key) {
		return 1;
	}
	return 0;
}
