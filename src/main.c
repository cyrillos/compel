#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <dlfcn.h>

#include <sys/types.h>

#include <getopt.h>

#include "compiler.h"
#include "version.h"

#include "common/log.h"
#include "uapi/libcompel.h"

int main(int argc, char *argv[])
{
	unsigned int loglevel = DEFAULT_LOGLEVEL;
	int opt, idx;

	char *path = NULL;
	pid_t pid = 0;

	void *handler = NULL;

	const char short_opts[] = "t:f:v:";
	static struct option long_opts[] = {
		{ "tree",		required_argument, 0, 't' },
		{ "file",		required_argument, 0, 'f' },
		{ "version",		no_argument, 0, 'V' },
		{ },
	};

	while (1) {
		opt = getopt_long(argc, argv, short_opts, long_opts, &idx);
		if (opt == -1)
			break;

		switch (opt) {
		case 't':
			pid = (pid_t)atol(optarg);
			break;
		case 'f':
			path = (optarg);
			break;
		case 'V':
			pr_msg("version %s id %s\n", COMPEL_VERSION, COMPEL_GITID);
			return 0;
			break;
		case 'v':
			if (optarg)
				loglevel = atoi(optarg);
			else
				loglevel++;
		default:
			break;
		}
	}

	loglevel_set(max(loglevel, (unsigned int)DEFAULT_LOGLEVEL));

	if (!pid || !path)
		goto usage;

	/*
	 * Try to use compel library.
	 *
	 * FIXME Hardcoded path for a while.
	 */
	handler = dlopen("src/lib/libcompel.so", RTLD_LAZY);
	if (!handler) {
		pr_err("Can't open compel library (%s)\n", dlerror());
		return -1;
	} else {
		lib_version_t libversion = dlsym(handler, LIB_VERSION_SYM);
		lib_seize_task_t libseizetask = dlsym(handler, LIB_SEIZE_TASK_SYM);
		lib_unseize_task_t libunseizetask = dlsym(handler, LIB_UNSEIZE_TASK_SYM);

		void *compel_data = NULL;

		if (libversion) {
			unsigned int major, minor;

			libversion(&major, &minor);
			pr_msg("libversion  : %d %d\n", major, minor);
		}

		if (libseizetask) {
			compel_data = libseizetask(pid);
			pr_msg("libseizetask: %p\n", compel_data);
		}

		if (libunseizetask) {
			compel_data = libseizetask(pid);
			pr_msg("libunseizetask: %d\n", libunseizetask(compel_data));
		}
	}

	dlclose(handler);
	return 0;

usage:
	pr_msg("\nUsage:\n");
	pr_msg("  %s run -t <pid> -f <file>\n", argv[0]);
	return -1;
}
