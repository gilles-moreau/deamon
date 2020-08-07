/* Skrum protocol socket */
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

#include "src/common/skrum_protocol_socket.h"

#define MAX_MSG_SIZE (1024*1024*1024)

static int _skrum_connect(int __fd, struct sockaddr const *__addr, socklen_t __len);

extern void skrum_setup_sockaddr(struct sockaddr_in *sin, uint16_t port, char *ip_addr)
{
	uint32_t s_addr;

	memset(sin, 0, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	if (inet_pton(AF_INET, ip_addr, &s_addr) < 0) {
		perror("ip address conversion");
		exit(1);
	}
	sin->sin_addr.s_addr = s_addr;
}

extern int skrum_init_discovery_engine(struct sockaddr_in *addr)
{
	int fd;
	int enable = 1;

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket discovery");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		    perror("setsockopt(SO_REUSEADDR) failed");

	if (bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr)) \
			== -1) {
		perror("bind discovery");
		return -1;
	}

	
	return fd;
}

extern int skrum_init_msg_engine(struct sockaddr_in *addr)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}

	if (bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr)) \
			== -1) {
		return -1;
	}

	if (listen(fd, BACKLOG) == -1) {
		return -1;
	}

	return fd;
}

extern int skrum_accept(int fd, struct sockaddr_in *addr)
{
	socklen_t len = sizeof(addr);
	int new_fd;
	if((new_fd = accept(fd, (struct sockaddr *)addr, &len)) == -1) {
		perror("accept");
	}
	return new_fd;
}

extern int skrum_msg_recvfrom(int fd, char **pbuf, uint32_t *lenp)
{
	ssize_t len;
	uint32_t msglen;

	len = skrum_recv(fd, (char *)&msglen, sizeof(msglen));
	if (len < ((ssize_t) sizeof(msglen))){
		return -1;
	}
	msglen = ntohl(msglen);

	if (msglen > MAX_MSG_SIZE)
		error("exceeded max message length");

	*pbuf = malloc(msglen);
	if (!pbuf)
		error("could not allocate memory");

	if (skrum_recv(fd, *pbuf, msglen) != msglen) {
		free(*pbuf);
		pbuf = NULL;
		return -1;
	}

	*lenp = msglen;

	return (ssize_t) msglen;
}

extern int skrum_msg_sendto(int fd, char *pbuf, uint32_t size)
{
	int len;
	uint32_t usize;

	usize = htonl(size);

	if ((len = skrum_send(fd, (char *)&usize, sizeof(usize))) < 0)
		return len;
	if ((len = skrum_send(fd, pbuf, size)) < 0)
		return len;
	return len;
}

extern int skrum_send(int fd, char *buf, uint32_t size)
{
	int rc;
	int sent = 0;

	while (sent < size) {
		rc = send(fd, &buf[sent], (size - sent), 0);
		if (rc < 0) {
			if (rc == EINTR)
				continue;
			error("unknown error in send");
			return -1;
		}

		if (rc == 0) {
			error("received empty message");
			return -1;
		}

		sent += rc;
	}

	if (sent != size) {
		error("sent size(%d) different from size(%d)",
				sent, size);
		return -1;
	}

	return rc;
}

extern int skrum_recv(int fd, char *buf, uint32_t size)
{
	int rc;
	int recved = 0;

	while (recved < size) {
		rc = recv(fd, &buf[recved], (size - recved), 0);
		if (rc < 0) {
			if (rc == EINTR)
				continue;
			perror("error in send");
			return -1;
		}

		if (rc == 0) {
			error("received empty message");
			return -1;
		}

		recved += rc;
	}

	if (recved != size) {
		error("received size(%d) different from size(%d)",
				recved, size);
		return -1;
	}

	return rc;
}

extern int skrum_open_stream(struct sockaddr_in *dest_addr)
{
	int fd, rc;

	if ( (dest_addr->sin_family == 0) || (dest_addr->sin_port == 0) ){
		error("error connecting, bad data: family = %u, port = %u",
				dest_addr->sin_family, dest_addr->sin_port);
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		error("error creating tcp socket");
		return -1;
	}
	
	rc = _skrum_connect(fd, (struct sockaddr const *)dest_addr, sizeof(*dest_addr));
	if (rc < 0) {
		perror("error while connecting to tcp remote socket");
		close(fd);
		return -1;
	}

	return fd;
}

static int _skrum_connect(int __fd, struct sockaddr const *__addr, socklen_t __len)
{
	int rc;

	rc = connect(__fd, __addr, __len);
	
	return rc;
}


extern int skrum_open_datagram_conn(struct sockaddr_in *dest_addr)
{
	int fd;

	if ( (dest_addr->sin_family == 0) || (dest_addr->sin_port == 0) ){
		error("error connecting, bad data: family = %u, port = %u",
				dest_addr->sin_family, dest_addr->sin_port);
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		error("error creating tcp socket");
		return -1;
	}
	/* no connection needed for datagram socket */

	return fd;
}
