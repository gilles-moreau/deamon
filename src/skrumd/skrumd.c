#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <hwloc.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_defs.h"
#include "src/common/skrum_protocol_pack.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_socket.h"
#include "src/common/macros.h"
#include "src/common/xcpuinfo.h"

#include "src/skrumd/skrumd_req.h"
#include "src/skrumd/skrumd.h"

#define LOCK_FILE "skrumd.lock"
#define LOG_FILE "skrumd.log"
#define RUNNING_DIR "/tmp"
#define MAXHOSTNAMELEN 1024

skrumd_conf_t *conf;
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;

static int _skrumd_init(void);
static void _init_conf(void);
static void _msg_engine(void);
static void _handle_connection(int sock);

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
	int sockfd;  /* listen on sock_fd, new connection on new_fd */
	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG };
	uint16_t port = 3434;
	uint16_t listening_port;

	//daemonize();

	/* init logs */
	init_log(argv[0], log_opt, LOG_FILE);

	/* init conf */
	conf = malloc(sizeof(skrumd_conf_t));
	if (!conf) {
		error("failed to malloc skrumd_conf_t");
		exit(1);
	}
	/* set default value for conf */
	_init_conf();
	conf->argc = &argc;
	conf->argv = &argv;

	if (_skrumd_init() < 0) {
		error("skrumd init failed");
		exit(1);
	}

	sockfd = skrum_init_msg_engine_port(port, &listening_port);
	if (sockfd < 0){
		error("engine msg");
		exit(1);
	}
	
	conf->port = listening_port;
	conf->lfd  = sockfd;
	conf->pid  = getpid();

	_msg_engine();

	return 0;
}

static void _msg_engine(void)
{
	struct sockaddr_in *cli = malloc(sizeof(struct sockaddr_in));
	int sock;

	while(1)
	{
		info("Waiting for new connection");
		if ((sock = skrum_accept(conf->lfd, cli)) >= 0){
			_handle_connection(sock);
			continue;
		}
		
		/* accept() failed */
		free(cli);
		error("accept()");
	}
	close(conf->lfd);
	return;
}

static void _handle_connection(int sock)
{
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));
	int rc = 0;

	skrum_msg_t_init(msg);
	if ((rc = skrum_recv_msg(sock, msg)) < 0){
		error("recv message");
		return;
	}
	skrumd_req(msg);

	return;
}

static int _skrumd_init(void) 
{
	int rc;
	
	rc = xcpuinfo_hwloc_topo_get(&conf->cpus, &conf->boards,
			&conf->sockets, &conf->cores, &conf->threads);
	if (rc) {
		error("failed to get hwloc info");
	}

	return rc;
}

static void _init_conf(void) 
{
	char host[MAXHOSTNAMELEN];
	size_t hostsize = 0;
	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG };

	host[MAXHOSTNAMELEN-1] = '\0';
	if (gethostname(host, hostsize)) {
		perror("could not get hostname");
		exit(1);
	}
	
	conf->hostname = host;
	conf->log_opts = log_opt;
	conf->debug_level = LOG_LEVEL_DEBUG;
	conf->lfd = -1;

	skrum_mutex_init(&conf->config_mutex);
	
	return;
}
