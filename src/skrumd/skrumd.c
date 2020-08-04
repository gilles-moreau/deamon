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
#include "src/common/skrum_discovery_api.h"
#include "src/common/macros.h"
#include "src/common/xcpuinfo.h"

#include "src/skrumd/skrumd_req.h"
#include "src/skrumd/skrumd.h"

#define LOG_FILE "skrumd.log"
#define MAXHOSTNAMELEN 1024

#define MAXTHREADS 4

typedef struct connection {
	int fd;
	struct sockaddr_in *cli_addr;
} conn_t;

/* count of active threads */
static int active_threads = 0;
static pthread_mutex_t active_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t active_cond  = PTHREAD_COND_INITIALIZER;

skrumd_conf_t *conf;
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;

static int _skrumd_init(void);
static void _init_conf(void);
static void _msg_engine(void);
static void _discovery_engine(void);
static void _handle_connection(int sock, struct sockaddr_in *cli);
static void _handle_discovery(skrum_msg_t *msg, struct sockaddr_in cont_addr);
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

	disc_sockfd = skrum_init_discovery();
	if (disc_sockfd < 0) {
		error("discovery engine msg");
		exit(1);
	}
	
	conf->port = listening_port;
	conf->lfd  = sockfd;
	conf->pid  = getpid();
	conf->disc_fd = disc_sockfd;

	//skrum_thread_create(&conf->thread_id_discovery, _discovery_engine, NULL);
	_discovery_engine();
	_msg_engine();

	return 0;
}

static void _discovery_engine(void)
{
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));
	struct sockaddr_in cont_addr;

	while(1)
	{
		info("waiting for multicast discovery msg");
		if (skrum_receive_discovery_msg(conf->disc_fd, msg, &cont_addr) == 0){
			_handle_discovery(msg, cont_addr);
			continue;
		}

		error("recv discovery failed");
	}
	close(conf->disc_fd);
	free(msg);
	return;
}

static void _handle_discovery(skrum_msg_t *msg, struct sockaddr_in cont_addr)
{
	discovery_msg_t *dmsg;
	char cont_ip_addr[INET_ADDRSTRLEN];

	if (!conf->registered || 
			(cont_addr.sin_addr.s_addr != conf->controller_ip.sin_addr.s_addr)) {
		/* set controller data if not registered */
		dmsg = (discovery_msg_t *)msg->data;
		conf->controller_ip = msg->orig_addr;
		inet_ntop(AF_INET, &conf->controller_ip.sin_addr, cont_ip_addr, sizeof(cont_ip_addr));
		info("controller ip is %s", cont_ip_addr);
		conf->cont_port = dmsg->controller_port;

		/* init request register message */
		skrum_msg_t register_msg;
		req_register_msg_t *req_msg; 
		int fd, rc;

		skrum_msg_t_init(&register_msg);
		req_msg = malloc(sizeof(req_register_msg_t));
		memset(req_msg, 0, sizeof(req_register_msg_t));
		req_msg->my_port = conf->port;

		register_msg.msg_type = REQUEST_REGISTER;
		register_msg.data = req_msg;
		
		fd = skrum_open_msg_conn(&cont_addr);
		if (fd < 0)
			return;
		
		rc = skrum_send_msg(fd, &register_msg);
		if (rc < 0)
			return;
		free(req_msg);
		close(fd);

	} else 
		info("deamon already registered");

	return;
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
	conn_t *arg = malloc(sizeof(conn_t));

	arg->fd = sock;
	arg->cli_addr = cli;

	_increment_thread_cnt();

	skrum_thread_create_detached(NULL, _service_connection, arg);

	return;
}

static void *_service_connection(void *arg)
{
	int rc = 0;
	conn_t *conn = (conn_t *)arg;
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));

	skrum_msg_t_init(msg);
	if ((rc = skrum_receive_msg(conn->fd, msg, conn->cli_addr)) < 0){
		error("recv message");
		return NULL;
	}
	skrumd_req(msg);

	free(msg);
	_decrement_thread_cnt();
	return NULL;

}

static void _decrement_thread_cnt(void)
{
	skrum_mutex_lock(&active_lock);
	if (active_threads>0)
		active_threads--;
	skrum_cond_signal(&active_cond);
	skrum_mutex_unlock(&active_lock);
}

static void _increment_thread_cnt(void)
{
	skrum_mutex_lock(&active_lock);
	while (active_threads >= MAXTHREADS) {
		skrum_cond_wait(&active_cond, &active_lock);
	}
	active_threads++;
	skrum_mutex_unlock(&active_lock);
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
