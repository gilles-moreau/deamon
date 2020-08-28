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
#include "src/common/xsignal.h"
#include "src/common/skrum_protocol_defs.h"
#include "src/common/skrum_protocol_pack.h"
#include "src/common/skrum_protocol_api.h"
#include "src/common/skrum_protocol_socket.h"
#include "src/common/skrum_discovery_api.h"
#include "src/common/macros.h"
#include "src/common/xcpuinfo.h"
#include "src/common/xsignal.h"

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

skrumd_conf_t *conf = NULL;
static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;

static int _shutdown = 0;

static int _skrumd_init(void);
static void _init_conf(void);
static void _msg_engine(void);
static void _skrumd_shutdown(int signo);
static int _skrumd_fini();
static void _destroy_conf(void);
static void *_discovery_engine(void *arg);
static void _handle_connection(int sock, struct sockaddr_in *cli);
static void *_service_connection(void *arg);
static void _increment_thread_cnt(void);
static void _decrement_thread_cnt(void);
static void _wait_for_all_threads(int secs);

int main(int argc, char **argv)
{
	int sockfd;  /* listen on sock_fd, new connection on new_fd */
	int disc_sockfd; /* listen on disc_sockfd for discovery */
	log_option_t log_opt = { LOG_LEVEL_DEBUG, LOG_LEVEL_QUIET, LOG_LEVEL_QUIET };
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

	xsignal(SIGINT, _skrumd_shutdown);
	xsignal(SIGTERM, _skrumd_shutdown);

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

	_increment_thread_cnt();
	skrum_thread_create(&conf->thread_id_discovery, _discovery_engine, NULL);
	_msg_engine();

	_wait_for_all_threads(120);
	_skrumd_fini();
	return 0;
}

static void *_discovery_engine(void *arg)
{
	struct sockaddr_in cont_addr;

	info("discovery engine msg on");
	while(!_shutdown)
	{
		skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));
		if (skrum_receive_discovery_msg(conf->disc_fd, msg, &cont_addr) == 0){
			skrumd_req(msg);
			skrum_free_msg_members(msg);
			free(msg);
			continue;
		}

		error("recv discovery failed");
	}

	close(conf->disc_fd);
	_decrement_thread_cnt();
	info("discovery engine is shut down");
	return NULL;
}

static void _msg_engine(void)
{
	struct sockaddr_in *cli = malloc(sizeof(struct sockaddr_in));
	int sock;

	while(!_shutdown)
	{
		info("waiting for new connection");
		if ((sock = skrum_accept(conf->lfd, cli)) >= 0){
			_handle_connection(sock, cli);
			continue;
		}
		
		/* accept() failed */
		free(cli);
		error("accept()");
	}
	info("message engine is shuting down");
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
	if ((rc = skrum_receive_msg(conn->fd, msg)) < 0){
		error("recv message");
		return NULL;
	}
	skrumd_req(msg);

	skrum_free_msg_members(msg);
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

static void _wait_for_all_threads(int secs)
{
	struct timespec ts;
	int rc;

	ts.tv_sec  = time(NULL);
	ts.tv_nsec = 0;
	ts.tv_sec += secs;

	skrum_mutex_lock(&active_lock);
	while (active_threads > 0) {
		info("waiting on %d active threads", active_threads);
		rc = pthread_cond_timedwait(&active_cond, &active_lock, &ts);
		if (rc == ETIMEDOUT) {
			error("Timeout waiting for completion of %d threads",
					active_threads);
			skrum_cond_signal(&active_cond);
			skrum_mutex_unlock(&active_lock);
			return;
		}
	}
	skrum_cond_signal(&active_cond);
	skrum_mutex_unlock(&active_lock);
	info("all threads complete");
}


static int _skrumd_init(void) 
{
	int rc;
	
	rc = xcpuinfo_init();
	if (rc) {
		error("failed to get hwloc info");
	}

	return rc;
}

static int _skrumd_fini() {
	xcpuinfo_fini();
	_destroy_conf();

	return 0;
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
	conf->cluster_id = 0;
	conf->ctrlr_ts = 0;

	skrum_mutex_init(&conf->config_mutex);
	
	return;
}

static void _destroy_conf(void)
{
	if (conf) {
		skrum_mutex_destroy(&conf->config_mutex);
		skrum_mutex_destroy(&active_lock);
		skrum_cond_destroy(&active_cond);
		free(conf);
	}
}

static void _skrumd_shutdown(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		_shutdown = 1;
	}
}
