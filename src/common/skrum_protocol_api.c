/* Skrum message send and recv API */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "src/common/skrum_protocol_pack.h"
#include "src/common/skrum_protocol_socket.h"
#include "src/common/logs.h"

#include "src/common/skrum_protocol_api.h"

extern int skrum_init_msg_engine_port(uint16_t selected_port, 
		uint16_t *actual_port)
{
	int cc;
	struct sockaddr_in addr;
	int i = selected_port;

	skrum_setup_sockaddr(&addr, selected_port, "0.0.0.0");
	cc = skrum_init_msg_engine(&addr);
	if ((cc < 0) || (selected_port = 0) || (errno = EADDRINUSE)){
		for (i=10001; i<65536; i++){
			skrum_setup_sockaddr(&addr, i, "0.0.0.0");
			cc = skrum_init_msg_engine(&addr);
			if (cc >= 0) {
				break;
			}
		}
	}
	*actual_port = i;
	return cc;
}

extern int skrum_send_msg(int fd, skrum_msg_t *msg)
{
	int rc;
	Buf buffer;

	/*
	 * Init buffer
	 */
	buffer = init_buf(BUF_SIZE);

	/* 
	 * Pack message
	 */
	pack_msg(msg, buffer);

	/*
	 * Send message
	 */
	rc = skrum_msg_sendto(fd, get_buf_data(buffer), get_buf_offset(buffer));

	if (rc < 0)
		error("error while sending message");

	free_buf(buffer);
	return rc;
}

int skrum_receive_msg(int fd, skrum_msg_t *msg)
{
	int rc;
	char *buf = NULL;
	uint32_t buflen = 0;
	Buf buffer;

	/* needed by skrumd_req to send back message */
	msg->conn_fd = fd;

	debug("Receiving message");
	/* Recv message. buf is allocated inside */
	rc = skrum_msg_recvfrom(fd, &buf, &buflen);
	if (rc < 0)
		error("error while receiving message");

	/* Create buffer */
	buffer = create_buf(buf, buflen);
	/* Unpack message */
	rc = unpack_msg(msg, buffer);

	free_buf(buffer);
	return rc;
}

extern int skrum_open_msg_conn(struct sockaddr_in *dest_addr)
{
	int fd = -1;
	
	if ((fd = skrum_open_stream(dest_addr)) <= 0) {
		error("could not open stream");
	}
	
	return fd;
}

extern int skrum_send_recv_msg(struct sockaddr_in dest_addr, 
		skrum_msg_t *request_msg, skrum_msg_t *response_msg)
{
	int rc = -1;
	int fd;

	if ((fd = skrum_open_msg_conn(&dest_addr)) < 0)
		return rc;

	if (skrum_send_msg(fd, request_msg) >= 0) 
		rc = skrum_receive_msg(fd, response_msg);
	
	close(fd);
	return rc;
}

