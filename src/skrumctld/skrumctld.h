#ifndef _SLURMCTLD_H
#define _SLURMCTLD_H

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "src/common/logs.h"
#include "src/common/list.h"

typedef struct skrumctld_config {
	char         *prog;		/* Program basename		   */
	char         ***argv;           /* pointer to argument vector      */
	int          *argc;             /* pointer to argument count       */
	char         *hostname;	 	/* local hostname		   */
	uint16_t     cpus;              /* lowest-level logical processors */
	uint16_t     boards;            /* total boards count              */
	uint16_t     sockets;           /* total sockets count             */
	uint16_t     threads;           /* thread per core count           */
	uint16_t     cores;             /* core per socket  count          */
	char         *node_topo_addr;   /* node's topology address         */
	char         *node_topo_pattern;/* node's topology address pattern */
	char         *logfile;		/* slurmd logfile, if any          */
	uint16_t     port;		/* local slurmctld port               */
	int          lfd;		/* skrumctld listen file descriptor   */
	int          disc_fd;           /* skrumctld discovery file desc      */
	uint16_t     disc_port;		/* skrumctld discovery port           */
	char         *mcast_ip;         /* multicast discovery ip addr     */	
	struct sockaddr_in controller_ip; /* controller ip		   */
	time_t       ctrlr_startup_ts;  /* controller startup time         */
	pthread_t    thread_id_discovery;  /* thread discovery id             */
	pid_t        pid;		/* server pid                      */
	log_option_t log_opts;         /* current logging options          */
	uint32_t     debug_level;	/* logging detail level            */
	pthread_mutex_t config_mutex;	/* lock for slurmd_config access   */
} skrumctld_conf_t;	

typedef struct cluster_node {
	uint16_t     cluster_node_id;
	time_t       registration_ts;  /* last registration timestamp      */
	uint16_t     rpc_port;         /* rpc listening port               */
} skrum_cluster_node_t;

#endif
