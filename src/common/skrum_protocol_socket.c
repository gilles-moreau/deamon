/* Skrum protocol socket */

#include "skrum_protocol_socket.h"

extern int skrum_init_msg_engine(struct sockaddr_in *addr)
{
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		return -1;
	}
	addr->sin_family = AF_INET;
	addr->sin_port = htons(PORT);
	addr->sin_addr.s_addr = INADDR_ANY;
	bzero(&(addr->sin_zero), 8);

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

extern int skrum_send(int fd, char *buf, size_t size)
{
	int rc;

	if ((rc = send(fd, buf, size, 0)) < 0)
		perror("send");
	close(fd);

	return rc;
}

extern int skrum_recv(int fd, char *buf, size_t size)
{
	int rc;

	if ((rc = recv(fd, buf, size, 0)) < 0)
		perror("recv");
	close(fd);

	return rc;
}
