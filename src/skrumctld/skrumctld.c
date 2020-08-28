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
#include "src/common/list.h"
#include "src/common/xsignal.h"

#include "src/skrumctld/skrumctld.h"
#include "src/skrumctld/skrumctld_req.h"

#define LOG_FILE "skrumctld.log"
#define MAXHOSTNAMELEN 1024
#define CONT_MAXTHREADS 4

#define NODE_REGISTRATION_TIMEOUT 10

typedef struct connection_arg {
	int fd;
	struct sockaddr_in *cli_addr;
} connection_arg_t;

/* count of active threads */
static int cont_active_threads = 0;
static pthread_mutex_t cont_active_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cont_active_cond  = PTHREAD_COND_INITIALIZER;

skrumctld_conf_t *conf;
List cluster_node_list = NULL;

static int _ctl_shutdown = 0;

static int _skrumctld_init(void);
static void _init_conf(void);
static void _msg_engine(void);
static void _skrumctld_shutdown(int signo);
static int _skrumctld_fini();
static void _destroy_conf(void);
static void *_discovery_engine(void *arg);
static int _setup_controller_addr(int fd, struct sockaddr_in *addr);
static void _update_node_list(List node_list);
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

	xsignal(SIGINT, _skrumctld_shutdown);
	xsignal(SIGTERM, _skrumctld_shutdown);

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
	_skrumctld_fini();

	return 0;
}

static void *_discovery_engine(void *arg)
{
	struct sockaddr_in addr;
	skrum_msg_t *msg = malloc(sizeof(skrum_msg_t));
	controller_info_msg_t *disc_msg = malloc(sizeof(controller_info_msg_t));

	skrum_msg_t_init(msg);

	msg->msg_type = MCAST_CONTROLLER_INFO; 
	_setup_controller_addr(conf->lfd, &addr);
	msg->orig_addr = addr;
	disc_msg->ctrlr_port = 3434;
	disc_msg->ctrlr_startup_ts = conf->ctrlr_startup_ts;
	msg->data = disc_msg;
	
	info("sending discovery multicast");
	while(!_ctl_shutdown) {
		if (skrum_send_discovery_msg(msg) < 0) 
			error("error sending discovery message");
		usleep(1000000);
		_update_node_list(cluster_node_list);
	}

	info("discovery engine is shut down");
	_decrement_thread_cnt();
	free(disc_msg);
	free(msg);
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

	while(!_ctl_shutdown)
	{
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
	if ((rc = skrum_receive_msg(conn->fd, msg)) < 0){
		error("recv message");
		return NULL;
	}
	skrumctld_req(msg);

	if ((conn->fd > 0) && (close(conn->fd) < 0))
		error("%s: error close fd", __func__);

	skrum_free_msg_members(msg);
	free(msg);
	free(conn);
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

static void _wait_for_all_threads(int secs)
{
	struct timespec ts;
	int rc;

	ts.tv_sec  = time(NULL);
	ts.tv_nsec = 0;
	ts.tv_sec += secs;

	skrum_mutex_lock(&cont_active_lock);
	while (cont_active_threads > 0) {
		info("waiting on %d active threads", cont_active_threads);
		rc = pthread_cond_timedwait(&cont_active_cond, &cont_active_lock, &ts);
		if (rc == ETIMEDOUT) {
			error("Timeout waiting for completion of %d threads",
					cont_active_threads);
			skrum_cond_signal(&cont_active_cond);
			skrum_mutex_unlock(&cont_active_lock);
			return;
		}
	}
	skrum_cond_signal(&cont_active_cond);
	skrum_mutex_unlock(&cont_active_lock);
	info("all threads complete");
}

static int _skrumctld_init(void) 
{
	int rc;
	
	rc = xcpuinfo_init();
	if (rc) {
		error("failed to get hwloc info");
	}

	cluster_node_list = list_create(free);
	conf->ctrlr_startup_ts = time(NULL);

	return rc;
}

static int _skrumctld_fini() {
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

	skrum_mutex_init(&conf->config_mutex);
	
	return;
}

static void _destroy_conf(void)
{
	if (conf) {
		list_destroy(cluster_node_list);
		skrum_mutex_destroy(&conf->config_mutex);
		skrum_mutex_destroy(&cont_active_lock);
		skrum_cond_destroy(&cont_active_cond);
		free(conf);
	}
}

static void _update_node_list(List node_list)
{
	ListIterator node_itr;
	skrum_cluster_node_t *node;
	time_t update_ts = time(NULL);

	node_itr = list_iterator_create(node_list);
	while((node = (skrum_cluster_node_t *)list_next(node_itr))) {	
		if ((update_ts - node->registration_ts) > 
				NODE_REGISTRATION_TIMEOUT) {
			list_remove(node_itr);
			info("node %d has been removed", node->cluster_node_id);
		}
	}

	list_iterator_destroy(node_itr);
	return;
}

static void _skrumctld_shutdown(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		_ctl_shutdown = 1;
	}
}

