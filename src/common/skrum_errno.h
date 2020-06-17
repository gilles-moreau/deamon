#ifndef _SKRUM_ERRNO_H
#define _SKRUM_ERRNO_H

#include <errno.h>

/* general return codes */
#define SKRUM_SUCCESS   0
#define SKRUM_ERROR    -1

enum {
	SKRUM_MSG_ERR = 1000,
	SKRUM_PCK_ERR,
	SKRUM_UPCK_ERR
};

/* look up an errno value */
char * skrum_strerror(int errnum);

#endif
