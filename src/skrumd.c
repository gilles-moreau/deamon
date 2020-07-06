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

#include "common/logs.h"
#include "common/skrum_protocol_defs.h"
#include "common/skrum_protocol_pack.h"
#include "common/skrum_protocol_api.h"
#include "common/skrum_protocol_socket.h"
#include "common/xcpuinfo.h"

#include "skrumd.h"

#define LOCK_FILE "skrumd.lock"
#define LOG_FILE "skrumd.log"
#define RUNNING_DIR "/tmp"

void signal_handler(int sig)
{
	switch(sig) {
		case SIGHUP:
			info("hangup signal catched");
			break;
		case SIGTERM:
			info("terminate signal catched");
			exit(0);
			break;
	}
}

void daemonize()
{
	int i,lfp;
	char str[10];

	if(getppid()==1) {
		info("Already a deamon");
		return; /* already a daemon */
	}

	i=fork();

	if (i<0) {
		error("Error: fork");
		exit(1); /* fork error */
	}
	if (i>0) 
		exit(0); /* parent exits */
	
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */

	for (i=getdtablesize();i>=0;--i) 
		close(i); /* close all descriptors */

	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */

	umask(0); /* set newly created file permissions */

	chdir(RUNNING_DIR); /* change running directory */

	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0640);

	if (lfp<0) {
		error("Error: cannot open lock file");
		exit(1); /* can not open */
	}
	if (lockf(lfp,F_TLOCK,0)<0) {
		error("Error: cannot open lock file");
		exit(0); /* can not lock */
	}

	/* first instance continues */
	sprintf(str,"%d\n",getpid());
	write(lfp,str,strlen(str)); /* record pid to lockfile */
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGHUP,signal_handler); /* catch hangup signal */
	signal(SIGTERM,signal_handler); /* catch kill signal */
}

int main(int argc, char **argv)
{
	int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
        struct sockaddr_in *my_addr;    /* my address information */
        struct sockaddr_in their_addr;    /* my address information */
	skrumd_conf_t *conf;
        int sin_size;
	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG };
	skrum_msg_t msg;

	my_addr = malloc(sizeof(my_addr));
	
	//daemonize();

	// Init logs
	init_log(argv[0], log_opt, LOG_FILE);

	xcpuinfo_hwloc_topo_get(&conf->cpus, &conf->boards,
			&conf->sockets, &conf->cores, &conf->threads);

	sockfd = skrum_init_msg_engine(my_addr);
	if (sockfd < 0){
		error("engine msg");
		exit(1);
	}

	info("Waiting for new connection");

	while(1)
	{
		new_fd = skrum_accept(sockfd, their_addr);

		info("server: got connection");
		
		skrum_recv_msg(new_fd, &msg);

		close(new_fd);  /* parent doesn't need this */

	}

	free(my_addr);
	return 0;
}
