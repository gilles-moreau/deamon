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

#include "src/common/logs.h"

#include "src/common/skrum_protocol_socket.h"

#define MAX_MSG_SIZE (1024*1024*1024)

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

extern int skrum_msg_recvfrom(int fd, char **pbuf, size_t *lenp)
{
	ssize_t len;
	uint32_t msglen;

	len = skrum_recv(fd, (char *)msglen, sizeof(msglen));
	if (len < ((ssize_t) sizeof(msglen))){
		return -1;
	}

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

extern int skrum_msg_sendto(int fd, char *pbuf, size_t size)
{
	int len;
	uint32_t usize;

	usize = htonl(size);

	if ((len = skrum_send(fd, (char *)&usize, sizeof(size))) < 0)
		return len;
	if ((len = skrum_send(fd, pbuf, size)) < 0)
		return len;
	return len;
}

extern int skrum_send(int fd, char *buf, size_t size)
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
	close(fd);
	return rc;
}

extern int skrum_recv(int fd, char *buf, size_t size)
{
	int rc;
	int recved = 0;

	while (recved < size) {
		rc = recv(fd, &buf[recved], (size - recved), 0);
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

		recved += rc;
	}

	if (recved != size) {
		error("received size(%d) different from size(%d)",
				recved, size);
		return -1;
	}
	close(fd);

	return rc;
}
