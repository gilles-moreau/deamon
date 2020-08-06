#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_pack.h"
#include "src/common/skrum_protocol_defs.h"
#include "src/common/skrum_protocol_socket.h"
#include "src/common/skrum_protocol_api.h"

#define DISCOVERY_PORT 6666
#define DISCOVERY_MCAST_ADDR "224.0.0.7"
#define UDP_BUF_SIZE 512

extern int skrum_init_discovery(void)
{
	int rc, fd;
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	uint32_t s_addr;

	skrum_setup_sockaddr(&addr, DISCOVERY_PORT, DISCOVERY_MCAST_ADDR);
	fd = skrum_init_discovery_engine(&addr);
	if (fd < 0) 
		error("could not bind socket");

	/* use setsocketopt to request the kernel to join multicast group */
	if ((rc = inet_pton(AF_INET, DISCOVERY_MCAST_ADDR, &s_addr)) < 0)
		perror("ip address conversion");

	mreq.imr_multiaddr.s_addr = s_addr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if ((rc = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq))) < 0)
		perror("setsocketopt");

	return fd;
}

extern int skrum_send_discovery_msg(skrum_msg_t *msg)
{
	int rc, fd;
	struct sockaddr_in dest_addr;
	Buf buffer;

	/* init buffer */	
	buffer = init_buf(UDP_BUF_SIZE);

	/* pack message */
	pack_msg(msg, buffer);

	/* send message */
	skrum_setup_sockaddr(&dest_addr, DISCOVERY_PORT, DISCOVERY_MCAST_ADDR);
	fd = skrum_open_datagram_conn(&dest_addr);
	if (fd < 0)
	       return -1;
	rc = sendto(fd, get_buf_data(buffer), get_buf_offset(buffer), 0, \
			(struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if (rc < 0)
		error("error while sending udp message");

	free_buf(buffer);
	close(fd);
	return rc;
}

extern int skrum_receive_discovery_msg(int fd, skrum_msg_t *msg, 
		struct sockaddr_in *cont_addr)
{
	int rc;
	char buf[UDP_BUF_SIZE];
	int addrlen;
	Buf buffer;

	/* receive message */
	addrlen = sizeof(cont_addr);
	rc = recvfrom(fd, buf, UDP_BUF_SIZE, 0, \
		       (struct sockaddr *)&cont_addr, (socklen_t *)&addrlen);
	if (rc < 0)
		error("error while receiving udp message");

	/* create buffer */
	buffer = create_buf(buf, UDP_BUF_SIZE);

	/* unpack message */
	rc = unpack_msg(msg, buffer);

	free_buf(buffer);
	return rc;
}
