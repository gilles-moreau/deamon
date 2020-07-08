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
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	sin->sin_addr.s_addr = INADDR_ANY;
	memset(sin, 0, sizeof(struct sockaddr_in));
}

extern int skrum_init_msg_engine_port(uint16_t selected_port, 
		uint16_t *actual_port)
{
	int cc;
	struct sockaddr_in addr;
	int i;

	skrum_setup_sockaddr(&addr, selected_port);
	cc = skrum_init_msg_engine(&addr);
	if ((cc < 0) && (selected_port = 0) && (errno = EADDRINUSE)){
		for (i=10001; i<65536; i++){
			skrum_setup_sockaddr(&addr, i);
			cc = skrum_init_msg_engine(&addr);
			if (cc >= 0) {
				*actual_port = i;
				break;
			}
		}
	}
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
	rc = skrum_send(fd, get_buf_data(buffer), size_buf(buffer));

	free_buf(buffer);
	return rc;
}

int skrum_recv_msg(int fd, skrum_msg_t *msg)
{
	int rc;
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
