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

extern void skrum_setup_sockaddr(struct sockaddr_in *sin, uint16_t port)
{
	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	sin->sin_addr.s_addr = INADDR_ANY;
}

extern int skrum_init_msg_engine_port(uint16_t selected_port, 
		uint16_t *actual_port)
{
	int cc;
	struct sockaddr_in addr;
	int i = selected_port;

	skrum_setup_sockaddr(&addr, selected_port);
	cc = skrum_init_msg_engine(&addr);
	if ((cc < 0) && (selected_port = 0) && (errno = EADDRINUSE)){
		for (i=10001; i<65536; i++){
			skrum_setup_sockaddr(&addr, i);
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
	info("Initializing buffer of size %d", BUF_SIZE);
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

int skrum_receive_msg(int fd, skrum_msg_t *msg, struct sockaddr_in *orig_addr)
{
	int rc;
	char *buf = NULL;
	uint32_t buflen = 0;
	Buf buffer;

	memcpy(&msg->orig_addr, orig_addr, sizeof(struct sockaddr_in));

	info("Receiving message");
	/*
	 * Recv message. buf is allocated inside
	 */
	rc = skrum_msg_recvfrom(fd, &buf, &buflen);
	if (rc < 0)
		error("error while receiving message");

	/*
	 * Create buffer
	 */
	buffer = create_buf(buf, buflen);
	/* 
	 * Unpack message
	 */
	rc = unpack_msg(msg, buffer);

	free_buf(buffer);
	return rc;
}
