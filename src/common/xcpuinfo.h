#ifndef _XCPUINFO_H_
#define _XCPUINFO_H_

#include <inttypes.h>

#define XCPUINFO_ERROR 1
#define XCPUINFO_SUCCESS 0

extern int xcpuinfo_hwloc_topo_get (
		uint16_t *p_cpus, uint16_t *p_boards,
		uint16_t *p_sockets, uint16_t *p_cores, uint16_t *p_threads);

int xcpuinfo_init(void);
int xcpuinfo_fini(void);

#endif
