#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include "compiler.h"
#include "ptrace.h"
#include "proc.h"

#include "common/log.h"

int unseize_task(pid_t pid, int st)
{
	pr_debug("\tUnseizing %d into %d\n", pid, st);

	if (st == TASK_DEAD)
		kill(pid, SIGKILL);
	else if (st == TASK_STOPPED)
		kill(pid, SIGSTOP);
	else if (st == TASK_ALIVE)
		/* do nothing */ ;
	else
		pr_err("Unknown final state %d\n", st);

	return ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

/*
 * This routine seizes task putting it into a special
 * state where we can manipulate the task via ptrace
 * interface, and finally we can detach ptrace out of
 * of it so the task would not know if it was saddled
 * up with someone else.
 */
int seize_task(pid_t pid, pid_t ppid, pid_t *pgid, pid_t *sid)
{
	struct proc_pid_stat_small ps;
	int ret, ret2, ptrace_errno;
	siginfo_t si;
	int status;

	ret = ptrace(PTRACE_SEIZE, pid, NULL, 0);
	ptrace_errno = errno;

	/*
	 * It's ugly, but the ptrace API doesn't allow to distinguish
	 * attaching to zombie from other errors. Thus we have to parse
	 * the target's /proc/pid/stat. Sad, but parse whatever else
	 * we might nead at that early point.
	 */
	ret2 = parse_pid_stat_small(pid, &ps);
	if (ret2 < 0)
		return -1;

	if (pgid)
		*pgid = ps.pgid;
	if (sid)
		*sid = ps.sid;

	if (ret < 0) {
		if (ps.state != 'Z') {
			if (pid == getpid())
				pr_err("SEIZE %d: me itself is within dumped tree\n", pid);
			else
				pr_err("SEIZE %d: Unseizable non-zombie found, state %c, err %d\n",
				       pid, ps.state, ret, ptrace_errno);
			return -1;
		}

		return TASK_DEAD;
	}

	if ((ppid != -1) && (ps.ppid != ppid)) {
		pr_err("SEIZE %d: task pid reused while seizing (%d -> %d)\n",
		       pid, ppid, ps.ppid);
		goto err;
	}

	ret = ptrace(PTRACE_INTERRUPT, pid, NULL, NULL);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't interrupt task", pid);
		goto err;
	}

try_again:
	ret = wait4(pid, &status, __WALL, NULL);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't wait task", pid);
		goto err;
	}

	if (ret != pid) {
		pr_err("SEIZE %d: wrong task attached (%d)\n", pid, ret);
		goto err;
	}

	if (!WIFSTOPPED(status)) {
		pr_err("SEIZE %d: task not stopped after seize\n", pid);
		goto err;
	}

	ret = ptrace(PTRACE_GETSIGINFO, pid, NULL, &si);
	if (ret < 0) {
		pr_perror("SEIZE %d: can't read signfo", pid);
		goto err;
	}

	if (SI_EVENT(si.si_code) != PTRACE_EVENT_STOP) {
		/*
		 * Kernel notifies us about the task being seized received some
		 * event other than the STOP, i.e. -- a signal. Let the task
		 * handle one and repeat.
		 */

		if (ptrace(PTRACE_CONT, pid, NULL, (void *)(unsigned long)si.si_signo)) {
			pr_perror("SEIZE %d: can't continue signal handling. Aborting", pid);
			goto err;
		}

		goto try_again;
	}

	if (si.si_signo == SIGTRAP)
		return TASK_ALIVE;
	else if (si.si_signo == SIGSTOP) {
		/*
		 * PTRACE_SEIZE doesn't affect signal or group stop state.
		 * Currently ptrace reported that task is in stopped state.
		 * We need to start task again, and it will be trapped
		 * immediately, because we sent PTRACE_INTERRUPT to it.
		 */
		ret = ptrace(PTRACE_CONT, pid, 0, 0);
		if (ret) {
			pr_perror("SEIZE %d: unable to start process", pid);
			goto err;
		}

		ret = wait4(pid, &status, __WALL, NULL);
		if (ret < 0) {
			pr_perror("SEIZE %d: can't wait task", pid);
			goto err;
		}

		if (ret != pid) {
			pr_err("SEIZE %d: wrong task attached (%d)\n", pid, ret);
			goto err;
		}

		if (!WIFSTOPPED(status)) {
			pr_err("SEIZE %d: task not stopped after seize\n", pid);
			goto err;
		}

		return TASK_STOPPED;
	}

	pr_err("SEIZE %d: unsupported stop signal %d\n", pid, si.si_signo);
err:
	unseize_task(pid, TASK_STOPPED);
	return -1;
}
