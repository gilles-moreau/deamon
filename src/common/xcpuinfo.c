#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include "logs.h"

#include <hwloc.h>

 
static void print_children(hwloc_topology_t topology, hwloc_obj_t obj,
		int depth)
{
	char type[32], attr[1024];
	unsigned i;

	hwloc_obj_type_snprintf(type, sizeof(type), obj, 0);
	printf("%*s%s", 2*depth, "", type);
	if (obj->os_index != (unsigned) -1)
		printf("#%u", obj->os_index);
	hwloc_obj_attr_snprintf(attr, sizeof(attr), obj, " ", 0);
	if (*attr)
		printf("(%s)", attr);
	printf("\n");
	for (i = 0; i < obj->arity; i++) {
		print_children(topology, obj->children[i], depth + 1);
	}
}

extern int xcpuinfo_hwloc_topo_get (
		uint16_t *p_cpus, uint16_t *p_boards,
		uint16_t *p_sockets, uint16_t *p_cores, uint16_t *p_threads)
{
	enum { SOCKET=0, CORE=1, PU=2, LAST_OBJ=3 };
	hwloc_topology_t topology;
	hwloc_cpuset_t cpuset;
	hwloc_obj_t obj;
	hwloc_obj_type_t objtype[LAST_OBJ];
	int nobj[LAST_OBJ];

	int actual_boards;
	int actual_cpus;
	int actual_cores;
	int sock_cnt;
	int cores_cnt;

	int depth;

	info("hwloc_topology_init");
	if (hwloc_topology_init(&topology)) {
		debug("hwloc_topology_init() failed.");
		return 1;
	}

	/* ignore cache and misc */
	hwloc_topology_set_flags(topology,
			HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_L1CACHE,
			HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_L2CACHE,
			HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_L3CACHE,
			HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_L4CACHE,
			HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_L5CACHE,
			HWLOC_TYPE_FILTER_KEEP_NONE);
	hwloc_topology_set_type_filter(topology, HWLOC_OBJ_MISC,
			HWLOC_TYPE_FILTER_KEEP_NONE);

	info("hwloc_topology_load");
	if (hwloc_topology_load(topology)) {
		debug("hwloc_topology_load() failed");
		return 2;
	}

	objtype[SOCKET] = HWLOC_OBJ_SOCKET;
	objtype[CORE]   = HWLOC_OBJ_CORE;
	objtype[PU]     = HWLOC_OBJ_PU;

	/* number of objects */
	depth = hwloc_get_type_depth(topology, HWLOC_OBJ_GROUP);
	if (depth != HWLOC_TYPE_DEPTH_UNKNOWN) {
		actual_boards = hwloc_get_nbobjs_by_depth(topology, depth);
	}

	/* number of sockets */
	depth = hwloc_get_type_depth(topology, objtype[SOCKET]);
	nobj[SOCKET] = hwloc_get_nbobjs_by_depth(topology, depth);

	/* number of cores */
	depth = hwloc_get_type_depth(topology, objtype[CORE]);
	actual_cores = hwloc_get_nbobjs_by_depth(topology, depth);

	/* number of cpu */
	depth = hwloc_get_type_depth(topology, objtype[PU]);
	actual_cpus = hwloc_get_nbobjs_by_depth(topology, depth);

	/* number of cores per socket */
	nobj[CORE] = actual_cores/nobj[SOCKET];

	/* number of threads per cores */
	nobj[PU] /= nobj[CORE];

	info("hwloc_topology_destroy");
	hwloc_topology_destroy(topology);

	/* set output values */
	*p_cpus = actual_cpus;
	*p_boards = actual_boards;
	*p_sockets = nobj[SOCKET];
	*p_cores = nobj[CORE];
	*p_threads = nobj[PU];

	return 0;
}
