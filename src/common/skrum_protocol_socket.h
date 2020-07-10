#ifndef _SKRUM_PROTOCOL_SOCKET_H
#define _SKRUM_PROTOCOL_SOCKET_H

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BACKLOG 10

extern int skrum_init_msg_engine(struct sockaddr_in *addr);
extern int skrum_accept(int fd, struct sockaddr_in *addr);
extern int skrum_msg_recvfrom(int fd, char **pbuf, size_t *size);
extern int skrum_msg_sendto(int fd, char *pbuf, size_t size);
extern int skrum_send(int fd, char *buf, size_t size);
extern int skrum_recv(int fd, char *buf, size_t size);

#endif
