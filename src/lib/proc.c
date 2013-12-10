#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "common/log.h"

#include "compiler.h"
#include "xmalloc.h"
#include "types.h"
#include "proc.h"
#include "bug.h"

static bool dir_dots(struct dirent *de)
{
	return !strcmp(de->d_name, ".") || !strcmp(de->d_name, "..");
}

int parse_threads(pid_t pid, pid_t **_t, int *_n)
{
	struct dirent *de;
	char path[128];
	pid_t *t = NULL;
	int nr = 1;
	DIR *dir;

	if (*_t)
		t = *_t;

	snprintf(path, sizeof(path), "/proc/%d/task", pid);
	dir = opendir(path);
	if (!dir) {
		pr_perror("Can't open %s", path);
		return -1;
	}

	while ((de = readdir(dir))) {
		pid_t *tmp;

		/* We expect numbers only here */
		if (de->d_name[0] == '.')
			continue;

		if (*_t == NULL) {
			tmp = xrealloc(t, nr * sizeof(pid_t));
			if (!tmp) {
				xfree(t);
				return -1;
			}
			t = tmp;
		}
		t[nr - 1] = atoi(de->d_name);
		nr++;
	}

	closedir(dir);

	if (*_t == NULL) {
		*_t = t;
		*_n = nr - 1;
	} else
		BUG_ON(nr - 1 != *_n);

	return 0;
}

int parse_children(pid_t pid, pid_t **_c, int *_n)
{
	char *tok, path[128];
	char buf[PAGE_SIZE];
	struct dirent *de;
	pid_t *ch = NULL;
	FILE *file;
	int nr = 1;
	DIR *dir;

	snprintf(path, sizeof(path), "/proc/%d/task", pid);
	dir = opendir(path);
	if (!dir) {
		pr_perror("Can't open %s", path);
		return -1;
	}

	while ((de = readdir(dir))) {
		if (dir_dots(de))
			continue;

		snprintf(path, sizeof(path), "/proc/%d/task/%s/children", pid, de->d_name);
		file = fopen(path, "r");
		if (!file) {
			pr_perror("Can't open %s", path);
			goto err;
		}

		if (!(fgets(buf, sizeof(buf), file)))
			buf[0] = 0;

		fclose(file);

		tok = strtok(buf, " \n");
		while (tok) {
			pid_t *tmp = xrealloc(ch, nr * sizeof(pid_t));
			if (!tmp)
				goto err;
			ch = tmp;
			ch[nr - 1] = atoi(tok);
			nr++;
			tok = strtok(NULL, " \n");
		}

	}

	*_c = ch;
	*_n = nr - 1;

	closedir(dir);
	return 0;
err:
	closedir(dir);
	xfree(ch);
	return -1;
}

int parse_pid_stat_small(pid_t pid, struct proc_pid_stat_small *s)
{
	char path[64], buf[4096];
	char *tok, *p;
	int fd, n;

	snprintf(path, sizeof(path), "/proc/%d/stat", pid);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	n = read(fd, buf, sizeof(buf));
	if (n < 1) {
		pr_err("stat for %d is corrupted\n", pid);
		close(fd);
		return -1;
	}
	close(fd);

	memzero(s, sizeof(*s));

	tok = strchr(buf, ' ');
	if (!tok)
		goto err;
	*tok++ = '\0';
	if (*tok != '(')
		goto err;

	s->pid = atoi(buf);

	p = strrchr(tok + 1, ')');
	if (!p)
		goto err;
	*tok = '\0';
	*p = '\0';

	strncpy(s->comm, tok + 1, sizeof(s->comm));

	n = sscanf(p + 1, " %c %d %d %d", &s->state, &s->ppid, &s->pgid, &s->sid);
	if (n < 4)
		goto err;

	return 0;
err:
	pr_err("Parsing %d's stat failed (#fields do not match)\n", pid);
	return -1;
}
