/* Skrum pack messages */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "skrum_protocol_pack.h"

void pack_ints_msg (ints_msg_t *msg, Buf buffer)
{
	pack16(msg->n_int16, buffer);
	pack32(msg->n_int32, buffer);
}

void unpack_ints_msg (ints_msg_t **msg_ptr, Buf buffer)
{
	info("Unpacking int message");
	ints_msg_t *msg;
	msg = malloc(sizeof(ints_msg_t));
	*msg_ptr = msg;

	memset(msg, 0, sizeof(ints_msg_t));
	unpack16(&msg->n_int16, buffer);
	unpack32(&msg->n_int32, buffer);
}

int pack_msg(skrum_msg_t const *msg, Buf buffer)
{
	int rc = 0;
	info("Packing message");

	pack16(msg->msg_type, buffer);
	pack_sockaddr(&msg->orig_addr, buffer);

	switch(msg->msg_type) {
		case INTS_MSG:
			pack_ints_msg((ints_msg_t *)msg->data, buffer);
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
		case INTS_MSG:
			unpack_ints_msg((ints_msg_t **)&(msg->data), buffer);
			break;
		default:
			rc = 1;
			printf("No upack method for this msg type");
	}

	return rc;
}
