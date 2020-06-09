/* Skrum message send and recv API */

#include "skrum_protocol_api.h"

int skrum_send_msg(int fd, skrum_msg_t *msg)
{
	int rc;
	Buf buffer;

	/*
	 * Init buffer
	 */
	info("Initializing buffer of size %d", BUF_SIZE);
	buffer = init_buf(BUF_SIZE);

	/* 
	 * Pack message
	 */
	pack_msg(msg, buffer);

	/*
	 * Send message
	 */
	rc = skrum_send(fd, get_buf_data(buffer), size_buf(buffer));

	free_buf(buffer);
	return rc;
}

int skrum_recv_msg(int fd, skrum_msg_t *msg)
{
	int rc;
	int buflen;
	char buf[MAX_BUF_SIZE];
	Buf buffer;

	info("Receiving message");
	/*
	 * Recv message
	 */
	rc = skrum_recv(fd, buf, MAX_BUF_SIZE);

	/*
	 * Create buffer
	 */
	buffer = create_buf(buf, MAX_BUF_SIZE);
	/* 
	 * Unpack message
	 */
	rc = unpack_msg(msg, buffer);

	free_buf(buffer);
	return rc;
}
