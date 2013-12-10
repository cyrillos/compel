#include <stdlib.h>
#include <stdio.h>

#include "pstree.h"
#include "ptrace.h"
#include "proc.h"
#include "xmalloc.h"

struct pstree_item *alloc_pstree_item(void)
{
	struct pstree_item *item = xzalloc(sizeof(*item));
	if (item) {
		INIT_LIST_HEAD(&item->children);
		INIT_LIST_HEAD(&item->sibling);
	}
	return item;
}

/* Deep first search on children */
struct pstree_item *pstree_item_next(struct pstree_item *item)
{
	if (!list_empty(&item->children))
		return list_first_entry(&item->children, struct pstree_item, sibling);

	while (item->parent) {
		if (item->sibling.next != &item->parent->children)
			return list_entry(item->sibling.next, struct pstree_item, sibling);
		item = item->parent;
	}

	return NULL;
}

void free_pstree(struct pstree_item *root_item)
{
	struct pstree_item *item = root_item, *parent;

	while (item) {
		if (!list_empty(&item->children)) {
			item = list_first_entry(&item->children, struct pstree_item, sibling);
			continue;
		}

		parent = item->parent;
		list_del(&item->sibling);
		xfree(item->threads);
		xfree(item);
		item = parent;
	}
}

static void unseize_task_and_threads(const struct pstree_item *item, int st)
{
	int i;

	for (i = 0; i < item->nr_threads; i++)
		unseize_task(item->threads[i], st);
}

static void pstree_switch_state(struct pstree_item *root_item, int st)
{
	struct pstree_item *item;

	pr_info("Unfreezing tasks into %d\n", st);
	for_each_pstree_item(root_item, item)
		unseize_task_and_threads(item, st);
}

static int check_subtree(const struct pstree_item *item)
{
	struct pstree_item *child;
	int nr, ret, i;
	pid_t *ch;

	ret = parse_children(item->pid, &ch, &nr);
	if (ret < 0)
		return ret;

	i = 0;
	list_for_each_entry(child, &item->children, sibling) {
		if (child->pid != ch[i])
			break;
		i++;
		if (i > nr)
			break;
	}
	xfree(ch);

	if (i != nr) {
		pr_info("Children set has changed while suspending\n");
		return -1;
	}

	return 0;
}

static pid_t item_ppid(const struct pstree_item *item)
{
	item = item->parent;
	return item ? item->pid : -1;
}

static int seize_threads(const struct pstree_item *item)
{
	int i = 0, ret;

	if ((item->state == TASK_DEAD) && (item->nr_threads > 1)) {
		pr_err("Zombies with threads are not supported\n");
		goto err;
	}

	for (i = 0; i < item->nr_threads; i++) {
		pid_t pid = item->threads[i];
		if (item->pid == pid)
			continue;

		pr_info("\tSeizing %d's %d thread\n", item->pid, pid);
		ret = seize_task(pid, item_ppid(item), NULL, NULL);
		if (ret < 0)
			goto err;

		if (ret == TASK_DEAD) {
			pr_err("Zombie thread not supported\n");
			goto err;
		}

		if (ret == TASK_STOPPED) {
			pr_err("Stopped threads not supported\n");
			goto err;
		}
	}

	return 0;

err:
	for (i--; i >= 0; i--) {
		if (item->pid == item->threads[i])
			continue;
		unseize_task(item->threads[i], TASK_ALIVE);
	}

	return -1;
}

static int check_threads(const struct pstree_item *item)
{
	pid_t *t = NULL;
	int nr, ret;

	ret = parse_threads(item->pid, &t, &nr);
	if (ret)
		return ret;

	ret = ((nr == item->nr_threads) && !memcmp(t, item->threads, nr));
	xfree(t);

	if (!ret) {
		pr_info("Threads set has changed while suspending\n");
		return -1;
	}

	return 0;
}

static int collect_threads(struct pstree_item *item)
{
	int ret;

	ret = parse_threads(item->pid, &item->threads, &item->nr_threads);
	if (!ret)
		ret = seize_threads(item);
	if (!ret)
		ret = check_threads(item);

	return ret;
}

static int get_children(struct pstree_item *item)
{
	int ret, i, nr_children;
	struct pstree_item *c;
	pid_t *ch;

	ret = parse_children(item->pid, &ch, &nr_children);
	if (ret < 0)
		return ret;

	for (i = 0; i < nr_children; i++) {
		c = alloc_pstree_item();
		if (!c) {
			ret = -1;
			goto free;
		}
		c->pid = ch[i];
		c->parent = item;
		list_add_tail(&c->sibling, &item->children);
	}
free:
	xfree(ch);
	return ret;
}

static int collect_task(struct pstree_item *item)
{
	pid_t pid = item->pid;
	int ret;

	ret = seize_task(pid, item_ppid(item), &item->pgid, &item->sid);
	if (ret < 0)
		goto err;

	pr_info("Seized task %d, state %d\n", pid, ret);
	item->state = ret;

	ret = collect_threads(item);
	if (ret < 0)
		goto err_close;

	ret = get_children(item);
	if (ret < 0)
		goto err_close;

	if ((item->state == TASK_DEAD) && !list_empty(&item->children)) {
		pr_err("Zombie with children?! O_o Run, run, run!\n");
		goto err_close;
	}

	pr_info("Collected %d in %d state\n", item->pid, item->state);
	return 0;

err_close:
	unseize_task(pid, item->state);
err:
	return -1;
}

static int collect_subtree(struct pstree_item *item)
{
	struct pstree_item *child;
	pid_t pid = item->pid;
	int ret;

	pr_info("Collecting tasks starting from %d\n", pid);
	ret = collect_task(item);
	if (ret)
		return -1;

	list_for_each_entry(child, &item->children, sibling) {
		ret = collect_subtree(child);
		if (ret < 0)
			return -1;
	}

	/*
	 * Tasks may clone() with the CLONE_PARENT flag while we collect
	 * them, making more kids to their parent. So before proceeding
	 * check that the parent we're working on has no more kids born.
	 */
	if (check_subtree(item))
		return -1;
	return 0;
}

int collect_pstree(compel_handler_t *handler, pid_t pid)
{
	int ret, attempts = 5;

	while (1) {
		handler->root = alloc_pstree_item();
		if (!handler->root)
			return -1;

		handler->root->pid = pid;

		ret = collect_subtree(handler->root);
		if (ret == 0) {
			/*
			 * Some tasks could have been reparented to
			 * namespaces' reaper. Check this.
			 */
			if (check_subtree(handler->root))
				goto try_again;
			break;
		}

		/*
		 * Old tasks can die and new ones can appear while we
		 * try to seize the swarm. It's much simpler (and reliable)
		 * just to restart the collection from the beginning
		 * rather than trying to chase them.
		 */
try_again:
		if (attempts == 0) {
			pr_err("Can't freeze the tree %d\n", pid);
			pstree_switch_state(handler->root, TASK_ALIVE);
			free_pstree(handler->root);
			handler->root = NULL;
			return -1;
		}

		attempts--;
		pr_info("Trying to suspend tasks %d again\n", pid);

		pstree_switch_state(handler->root, TASK_ALIVE);
		free_pstree(handler->root);
		handler->root = NULL;
	}

	return 0;
}
