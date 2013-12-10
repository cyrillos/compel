#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "common/log.h"

#include "uapi/libcompel.h"

#include "libcompel.h"
#include "compiler.h"
#include "version.h"
#include "xmalloc.h"
#include "pstree.h"
#include "ptrace.h"
#include "types.h"
#include "err.h"

void lib_version(unsigned int *major, unsigned int *minor)
{
	*major = COMPEL_VERSION_MAJOR;
	*minor = COMPEL_VERSION_MINOR;
}

void *lib_seize_task(pid_t pid)
{
	compel_handler_t *h;
	int ret;

	h = xzalloc(sizeof(*h));
	if (!h)
		return NULL;

	ret = collect_pstree(h, pid);
	if (ret) {
		xfree(h);
		return NULL;
	}

	return (void *)h;
}

int lib_unseize_task(void *seize_data)
{
	compel_handler_t *h = seize_data;
	if (h)
		pstree_switch_state(h->root, TASK_ALIVE);
	return 0;
}

int lib_infect_task(void *seize_data)
{
	return -EINVAL;
}

int lib_cure_task(void *seize_data)
{
	return -EINVAL;
}

int lib_exec(void *seize_data)
{
	return -EINVAL;
}
