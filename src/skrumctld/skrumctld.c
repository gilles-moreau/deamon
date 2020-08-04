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
#include <net/if.h>
#include <netdb.h>

#include <sys/ioctl.h>

#include "src/common/logs.h"
#include "src/common/skrum_protocol_defs.h"
#include "src/common/skrum_protocol_pack.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_socket.h"
#include "src/common/skrum_discovery_api.h"
#include "src/common/macros.h"
#include "src/common/xcpuinfo.h"

#include "src/skrumctld/skrumctld.h"
#include "src/skrumctld/skrumctld_req.h"

#define LOG_FILE "skrumctld.log"
#define MAXHOSTNAMELEN 1024
#define CONT_MAXTHREADS 4

typedef struct connection_arg {
	int fd;
	struct sockaddr_in *cli_addr;
} connection_arg_t;

/* count of active threads */
static int cont_active_threads = 0;
static pthread_mutex_t cont_active_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cont_active_cond  = PTHREAD_COND_INITIALIZER;

skrumctld_conf_t *conf;

static int _skrumctld_init(void);
static void _init_conf(void);
static void _msg_engine(void);
static void *_discovery_engine(void *arg);
static int _setup_controller_addr(int fd, struct sockaddr_in *addr);
static void _handle_connection(int sock, struct sockaddr_in *cli);
static void *_service_connection(void *arg);
static void _increment_thread_cnt(void);
static void _decrement_thread_cnt(void);

int main(int argc, char **argv)
{
	int sockfd;  /* listen on sock_fd, new connection on new_fd */
	int disc_sockfd; /* listen on disc_sockfd for discovery */
	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG };
	uint16_t port = 3434;
	uint16_t listening_port;

	//daemonize();

	/* init logs */
	init_log(argv[0], log_opt, LOG_FILE);

	/* init conf */
	conf = malloc(sizeof(skrumctld_conf_t));
	if (!conf) {
		error("failed to malloc skrumctld_conf_t");
		exit(1);
	}
	/* set default value for conf */
	_init_conf();
	conf->argc = &argc;
	conf->argv = &argv;

	if (_skrumctld_init() < 0) {
		error("skrumctld init failed");
		exit(1);
	}

	sockfd = skrum_init_msg_engine_port(port, &listening_port);
	if (sockfd < 0){
		error("engine msg");
		exit(1);
	}

	disc_sockfd = skrum_init_discovery();
	if (disc_sockfd < 0) {
		error("discovery engine msg");
		exit(1);
	}
	
	conf->port = listening_port;
	conf->lfd  = sockfd;
	conf->pid  = getpid();
	conf->disc_fd = disc_sockfd;

	skrum_thread_create(&conf->thread_id_discovery, _discovery_engine, NULL);
	_msg_engine();

	return 0;
}

static void *_discovery_engine(void *arg)
{
	struct sockaddr_in addr;
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));
	discovery_msg_t *disc_msg = malloc(sizeof(discovery_msg_t));

	skrum_msg_t_init(msg);

	msg->msg_type = MCAST_DISCOVERY; 
	_setup_controller_addr(conf->lfd, &addr);
	msg->orig_addr = addr;
	disc_msg->controller_port = 3434;
	msg->data = disc_msg;
	
	while(1) {
		info("sending discovery multicast");
		if (skrum_send_discovery_msg(msg) < 0) 
			error("error sending discovery message");
		usleep(1000000);
	}

	free(msg);
	free(disc_msg);
	return NULL;
}

static int _setup_controller_addr(int fd, struct sockaddr_in *addr)
{
	int rc = 0;
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "lo", IFNAMSIZ-1);
	if ((rc = ioctl(fd, SIOCGIFADDR, &ifr)) < 0)
	       error("ioctl, get ip address");

	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(conf->port);
	addr->sin_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

	return rc;
}

static void _msg_engine(void)
{
	struct sockaddr_in *cli = malloc(sizeof(struct sockaddr_in));
	int sock;

	while(1)
	{
		info("Waiting for new connection");
		if ((sock = skrum_accept(conf->lfd, cli)) >= 0){
			_handle_connection(sock, cli);
			continue;
		}
		
		/* accept() failed */
		free(cli);
		error("accept()");
	}
	close(conf->lfd);
	return;
}

static void _handle_connection(int sock, struct sockaddr_in *cli)
{
	connection_arg_t *arg = malloc(sizeof(connection_arg_t));

	arg->fd = sock;
	arg->cli_addr = cli;

	_increment_thread_cnt();

	skrum_thread_create_detached(NULL, _service_connection, arg);

	return;
}

static void *_service_connection(void *arg)
{
	int rc = 0;
	connection_arg_t *conn = (connection_arg_t *)arg;
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));

	skrum_msg_t_init(msg);
	if ((rc = skrum_receive_msg(conn->fd, msg, conn->cli_addr)) < 0){
		error("recv message");
		return NULL;
	}
	skrumctld_req(msg);

	free(msg);
	_decrement_thread_cnt();
	return NULL;

}

static void _decrement_thread_cnt(void)
{
	skrum_mutex_lock(&cont_active_lock);
	if (cont_active_threads>0)
		cont_active_threads--;
	skrum_cond_signal(&cont_active_cond);
	skrum_mutex_unlock(&cont_active_lock);
}

static void _increment_thread_cnt(void)
{
	skrum_mutex_lock(&cont_active_lock);
	while (cont_active_threads >= CONT_MAXTHREADS) {
		skrum_cond_wait(&cont_active_cond, &cont_active_lock);
	}
	cont_active_threads++;
	skrum_mutex_unlock(&cont_active_lock);
}

static int _skrumctld_init(void) 
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
	size_t hostsize = MAXHOSTNAMELEN;
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

//int main(int argc, char **argv)
//{
//	int sockfd;  /* listen on sock_fd, new connection on new_fd */
//	struct addrinfo hints, *res;
//	int status;
//
//	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG };
//
//	skrum_msg_t msg;
//	ints_msg_t *ints_msg;
//	//daemonize();
//	
//	// Init logs
//	init_log(argv[0], log_opt, LOG_FILE);
//
//	memset(&hints, 0, sizeof(struct addrinfo));
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
//	hints.ai_flags = AF_INET; // fill in my IP for me
//
//	info("Starting connection");
//	
//	if ((status = getaddrinfo("127.0.0.1" , PORT, &hints, &res)) != 0) {
//		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
//		return 2;
//	}
//
//	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
//		perror("socket");
//		exit(1);
//	}
//	
//	if ((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) == -1) {
//		perror("connect");
//		exit(1);
//	}
//
//	/*
//	 * Init message
//	 */	
//	msg.msg_type = INTS_MSG;
//	msg.orig_addr = *(struct sockaddr_in *)res->ai_addr;
//	ints_msg = malloc(sizeof(ints_msg_t));
//	ints_msg->n_int16 = 16;	
//	ints_msg->n_int32 = 32;
//	msg.data = ints_msg;
//	msg.data_size = sizeof(ints_msg_t);
//
//	if (skrum_send_msg(sockfd, &msg) < 0) {
//		perror("Skrum send");
//		exit(1);
//	}	
//
//	return 0;
//}
