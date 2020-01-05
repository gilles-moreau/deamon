#ifndef _MACROS_H
#define _MACROS_H

#include <errno.h>              /* for errno   */
#include <pthread.h>
#include <string.h>

#include "logs.h"

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define skrum_mutex_init(mutex)						\
	do {								\
		int err = pthread_mutex_init(mutex, NULL);		\
		if (err) {						\
			errno = err;					\
			fatal("%s:%d %s: pthread_mutex_init(): %m",	\
				__FILE__, __LINE__, __func__);		\
			abort();					\
		}							\
	} while (0)

#define skrum_mutex_destroy(mutex)					\
	do {								\
		int err = pthread_mutex_destroy(mutex);			\
		if (err) {						\
			errno = err;					\
			fatal("%s:%d %s: pthread_mutex_destroy(): %m",	\
				__FILE__, __LINE__, __func__);		\
			abort();					\
		}							\
	} while (0)

#define skrum_mutex_lock(mutex)					\
	do {								\
		int err = pthread_mutex_lock(mutex);			\
		if (err) {						\
			errno = err;					\
			fatal("%s:%d %s: pthread_mutex_lock(): %m",	\
				__FILE__, __LINE__, __func__);		\
			abort();					\
		}							\
	} while (0)

#define skrum_mutex_unlock(mutex)					\
	do {								\
		int err = pthread_mutex_unlock(mutex);			\
		if (err) {						\
			errno = err;					\
			fatal("%s:%d %s: pthread_mutex_unlock(): %m",	\
				__FILE__, __LINE__, __func__);		\
			abort();					\
		}							\
	} while (0)

#endif
