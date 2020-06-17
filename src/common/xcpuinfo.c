#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>

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
	int depth, i;
	int topodepth;	
	char string[128];
	hwloc_topology_t topology;
	hwloc_cpuset_t cpuset;
	hwloc_obj_t obj;

	hwloc_topology_init(&topology);
	print_children(topology, hwloc_get_root_obj(topology), 0);

	return 0;
}
