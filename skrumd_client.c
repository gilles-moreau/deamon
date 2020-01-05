#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include "common/skrum_protocol_defs.h"
#include "common/skrum_protocol_pack.h"
#include "common/skrum_protocol_api.h"

#define PORT "3434"
#define BACKLOG 10
#define MAX_L_HOSTNAME 10

int main(int argc, char **argv)
{
	int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
	struct addrinfo hints, *res, *p;
	struct addrinfo *servinfo;
        struct sockaddr_in their_addr; /* connector's address information */
        int sin_size;
	int status;
	char hostname[MAX_L_HOSTNAME+1];
	int host_size = MAX_L_HOSTNAME;

	uint32_t buffer_size = BUF_SIZE;
	skrum_msg_t msg;
	ints_msg_t *ints_msg;
	//daemonize();

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AF_INET; // fill in my IP for me
	
	if ((status = getaddrinfo("127.0.0.1" , PORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		perror("socket");
		exit(1);
	}
	
	if ((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
		perror("connect");
		exit(1);
	}

	printf("Starting connection\n");

	/*
	 * Init message
	 */	
	msg.msg_type = INTS_MSG;
	msg.dst_addr = their_addr;
	ints_msg = malloc(sizeof(ints_msg_t));
	ints_msg->n_int16 = 16;	
	ints_msg->n_int32 = 32;
	msg.data = ints_msg;

	if (skrum_send_msg(sockfd, &msg) < 0) {
		perror("Skrum send");
		exit(1);
	}	

	return 0;
}
