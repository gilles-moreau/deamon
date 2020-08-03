#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "src/common/logs.h"

#define LOCK_FILE "skrumd.lock"
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
