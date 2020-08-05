/* Skrum pack messages */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "src/common/pack.h"
#include "src/common/logs.h"
#include "src/common/skrum_protocol_api.h"

#include "src/common/skrum_protocol_pack.h"

void pack_discovery_msg(discovery_msg_t *msg, Buf buffer)
{
	pack16(msg->controller_port, buffer);
}

void unpack_discovery_msg(discovery_msg_t **msg_ptr, Buf buffer)
{
	discovery_msg_t *msg;
	msg = malloc(sizeof(discovery_msg_t));
	*msg_ptr = msg;

	memset(msg, 0, sizeof(discovery_msg_t));
	unpack16(&msg->controller_port, buffer);
}

void pack_request_registration_msg(req_register_msg_t *msg, Buf buffer)
{
	pack16(msg->my_port, buffer);
	pack16(msg->my_id, buffer);
}

void unpack_request_registration_msg(req_register_msg_t **msg_ptr, Buf buffer)
{
	req_register_msg_t *msg;
	msg = malloc(sizeof(req_register_msg_t));
	*msg_ptr = msg;

	memset(msg, 0, sizeof(req_register_msg_t));
	unpack16(&msg->my_port, buffer);
	unpack16(&msg->my_id, buffer);
}

void pack_response_registration_msg(resp_register_msg_t *msg, Buf buffer)
{
	pack16(msg->my_id, buffer);
}

void unpack_response_registration_msg(resp_register_msg_t **msg_ptr, Buf buffer)
{
	resp_register_msg_t *msg;
	msg = malloc(sizeof(resp_register_msg_t));
	*msg_ptr = msg;

	memset(msg, 0, sizeof(req_register_msg_t));
	unpack16(&msg->my_id, buffer);
}

int pack_msg(skrum_msg_t const *msg, Buf buffer)
{
	int rc = 0;

	pack16(msg->msg_type, buffer);
	pack_sockaddr(&msg->orig_addr, buffer);

	switch(msg->msg_type) {
		case MCAST_DISCOVERY:
			pack_discovery_msg((discovery_msg_t *)msg->data, buffer);
			pack32(msg->data_size, buffer);
			break;
		case REQUEST_NODE_REGISTRATION:
			pack_request_registration_msg((req_register_msg_t *)msg->data, buffer);
			pack32(msg->data_size, buffer);
			break;
		case RESPONSE_NODE_REGISTRATION:
			pack_response_registration_msg((resp_register_msg_t *)msg->data, buffer);
			pack32(msg->data_size, buffer);
			break;
		default:
			rc = 1;
			error("No pack method for the msg type");
	}

	return rc;
}

int unpack_msg(skrum_msg_t *msg, Buf buffer)
{
	int rc = 0;

	info("Unpacking message");
	msg->data = NULL;

	unpack16(&msg->msg_type, buffer);
	unpack_sockaddr(&msg->orig_addr, buffer);

	switch (msg->msg_type) {
		case MCAST_DISCOVERY:
			unpack_discovery_msg((discovery_msg_t **)&(msg->data), buffer);
			break;
		case REQUEST_NODE_REGISTRATION:
			unpack_request_registration_msg((req_register_msg_t **)msg->data, buffer);
			pack32(msg->data_size, buffer);
			break;
		case RESPONSE_NODE_REGISTRATION:
			unpack_response_registration_msg((resp_register_msg_t **)msg->data, buffer);
			pack32(msg->data_size, buffer);
			break;
		default:
			rc = 1;
			error("No unpack method for this msg type");
	}

	return rc;
}
