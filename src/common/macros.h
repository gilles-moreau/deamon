#ifndef _MACROS_H
#define _MACROS_H

#include <errno.h>              /* for errno   */
#include <pthread.h>
#include <string.h>

#include "src/common/logs.h"

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define skrum_mutex_init(mutex)						        \
	do {								        \
		int err = pthread_mutex_init(mutex, NULL);		        \
		if (err) {						        \
			errno = err;					        \
			fatal("%s:%d %s: pthread_mutex_init(): %m",	        \
				__FILE__, __LINE__, __func__);		        \
			abort();					        \
		}							        \
	} while (0)

#define skrum_mutex_destroy(mutex)					        \
	do {								        \
		int err = pthread_mutex_destroy(mutex);			        \
		if (err) {						        \
			errno = err;					        \
			fatal("%s:%d %s: pthread_mutex_destroy(): %m",	        \
				__FILE__, __LINE__, __func__);		        \
			abort();					        \
		}							        \
	} while (0)

#define skrum_mutex_lock(mutex)					                \
	do {								        \
		int err = pthread_mutex_lock(mutex);			        \
		if (err) {						        \
			errno = err;					        \
			fatal("%s:%d %s: pthread_mutex_lock(): %m",	        \
				__FILE__, __LINE__, __func__);		        \
			abort();					        \
		}							        \
	} while (0)

#define skrum_mutex_unlock(mutex)					        \
	do {								        \
		int err = pthread_mutex_unlock(mutex);			        \
		if (err) {						        \
			errno = err;					        \
			fatal("%s:%d %s: pthread_mutex_unlock(): %m",	        \
				__FILE__, __LINE__, __func__);		        \
			abort();					        \
		}							        \
	} while (0)

#endif

#define skrum_thread_create(id, func, arg)				        \
	do {								        \
		pthread_attr_t attr;                                            \
		int err;                                                \
		skrum_attr_init(&attr);                                 \
		err = pthread_create(id, &attr, func, arg);             \
		if (err) {                                              \
			errno = err;                                    \
			fatal("%s: pthread_create error %m", __func__); \
		}                                                       \
		skrum_attr_destroy(&attr);                                      \
	} while (0)

#define skrum_thread_create_detached(id, func, arg)                             \
	do {                                                                    \
		pthread_t *id_ptr, id_local;                                    \
		pthread_attr_t attr;                                            \
		int err;                                                        \
		id_ptr = (id != (pthread_t *) NULL) ? id : &id_local;           \
		skrum_attr_init(&attr);                                         \
		err = pthread_attr_setdetachstate(&attr,                        \
				PTHREAD_CREATE_DETACHED); \
		if (err) {                                                      \
			errno = err;                                            \
			fatal("%s: pthread_attr_setdetachstate %m",  __func__); \
		}                                                               \
		err = pthread_create(id_ptr, &attr, func, arg);                 \
		if (err) {                                                      \
			errno = err;                                            \
			fatal("%s: pthread_create error %m", __func__);         \
		}                                                               \
		skrum_attr_destroy(&attr);                                      \
	} while (0)

#define skrum_attr_init(attr)                                                   \
	do {                                                                    \
		int err = pthread_attr_init(attr);                              \
		if (err) {                                                      \
			errno = err;                                            \
			fatal("pthread_attr_init: %m");                         \
		}                                                               \
		err = pthread_attr_setstacksize(attr, 1024*1024);               \
		if (err) {                                                      \
			errno = err;                                            \
			error("pthread_attr_setstacksize: %m");                 \
		}                                                               \
	} while (0)

#define skrum_attr_destroy(attr)                                                \
	do {                                                                    \
		int err = pthread_attr_destroy(attr);                           \
		if (err) {                                                      \
			errno = err;                                            \
			error("pthread_attr_destroy failed, "                   \
					"possible memory leak!: %m");           \
		}                                                               \
	} while (0)

#define skrum_cond_init(cond, cont_attr)                                \
	do {                                                            \
		int err = pthread_cond_init(cond, cont_attr);           \
		if (err) {                                              \
			errno = err;                                    \
			fatal("%s:%d %s: pthread_cond_init(): %m",      \
					__FILE__, __LINE__, __func__);          \
			abort();                                        \
		}                                                       \
	} while (0)

#define skrum_cond_signal(cond)                                 \
	do {                                                            \
		int err = pthread_cond_signal(cond);                    \
		if (err) {                                              \
			errno = err;                                    \
			error("%s:%d %s: pthread_cond_signal(): %m",    \
					__FILE__, __LINE__, __func__);          \
		}                                                       \
	} while (0)

#define skrum_cond_wait(cond, mutex)                                    \
	do {                                                            \
		int err = pthread_cond_wait(cond, mutex);               \
		if (err) {                                              \
			errno = err;                                    \
			error("%s:%d %s: pthread_cond_wait(): %m",      \
					__FILE__, __LINE__, __func__);          \
		}                                                       \
	} while (0)
