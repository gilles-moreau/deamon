#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skrum_errno.h"

/* Type for error string table entries */
typedef struct {
	int xe_number;
	char *xe_message;
} skrum_errtab_t;

static skrum_errtab_t skrum_errtab[] = {
	{ SKRUM_MSG_ERR, 
	  "Message error"},
	{ SKRUM_PCK_ERR,
	  "Error while packing message"},
	{ SKRUM_UPCK_ERR,
	  "Error while unpacking message"}
};

/*
 * Linear search through table of errno values and strings,
 * returns NULL on error, string on success.
 */
static char *_lookup_skrum_api_errtab(int errnum)
{
	char *res = NULL;
	int i;

	for (i = 0; i < sizeof(skrum_errtab) / sizeof(skrum_errtab_t); i++) {
		if (skrum_errtab[i].xe_number == errnum) {
			res = skrum_errtab[i].xe_message;
			break;
		}
	}

	return res;
}

/*
 * Return string associated with error (Slurm or system).
 * Always returns a valid string (because strerror always does).
 */
char *skrum_strerror(int errnum)
{
	char *res = _lookup_skrum_api_errtab(errnum);
	if (res)
		return res;
	else if (errnum > 0)
		return strerror(errnum);
	else
		return "Unknown negative error number";
}
