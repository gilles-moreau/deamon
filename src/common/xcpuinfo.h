#ifndef _XCPUINFO_H_
#define _XCPUINFO_H_

extern int xcpuinfo_hwloc_topo_get (
		uint16_t *p_cpus, uint16_t *p_boards,
		uint16_t *p_sockets, uint16_t *p_cores, uint16_t *p_threads);

#endif
