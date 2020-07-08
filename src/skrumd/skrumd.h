#ifndef _SLURMD_H
#define _SLURMD_H

#include <inttypes.h>
#include <pthread.h>

#include "src/common/logs.h"

typedef struct skrumd_config {
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
	uint32_t     syslog_debug;	/* send output to both logfile and
					 * syslog */
	uint16_t      port;		/* local slurmd port               */
	int           lfd;		/* slurmd listen file descriptor   */
	pid_t         pid;		/* server pid                      */
	log_option_t  log_opts;         /* current logging options         */
	uint32_t      debug_level;	/* logging detail level            */
	pthread_mutex_t config_mutex;	/* lock for slurmd_config access   */
} skrumd_conf_t;	

#endif
