#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>

typedef struct {
	int	no;
	char	msg[64];
} tdata_t;

static void thread_func(void *p)
{
	tdata_t *d = (tdata_t *)p;

	printf("Thread [%d]: %s\n", d->no, d->msg);
	while (1)
		sleep(10);

	pthread_exit(0);
}

int main(int argc, char *argv)
{
	pthread_t th1, th2;
	tdata_t d1, d2;

	d1.no = 1;
	strcpy(d1.msg, "Message 1");

	d2.no = 2;
	strcpy(d2.msg, "Message 2");

	pthread_create(&th1, NULL, (void *)&thread_func, (void *)&d1);
	pthread_create(&th2, NULL, (void *)&thread_func, (void *)&d2);

	pthread_join(th1, NULL);
	pthread_join(th2, NULL);

	exit(0);
}
