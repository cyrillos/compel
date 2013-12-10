#ifndef __COMPEL_PSTREE_H__
#define __COMPEL_PSTREE_H__

#include "list.h"
#include "libcompel.h"

#define INIT_PID	(1)

struct pstree_item {
	struct pstree_item	*parent;

	struct list_head	children;	/* list of my children */
	struct list_head	sibling;	/* linkage in my parent's children list */

	pid_t			pid;
	pid_t			pgid;
	pid_t			sid;

	int			state;		/* TASK_XXX constants */

	int			nr_threads;	/* number of threads */
	pid_t			*threads;	/* array of threads */
};

extern struct pstree_item *alloc_pstree_item(void);
extern int collect_pstree(compel_handler_t *handler, pid_t pid);
extern void pstree_switch_state(struct pstree_item *root_item, int st);

extern struct pstree_item *pstree_item_next(struct pstree_item *item);
#define for_each_pstree_item(root, pi)		\
	for (pi = root_item; pi; pi = pstree_item_next(pi))

#endif /* __COMPEL_PSTREE_H__ */
