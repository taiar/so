#include <stdlib.h>
#include <stdio.h>

#include "dccthread.h"

void f1(int dummy) {
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 0;

	printf("\n");
	int i;
	printf("contando até 5\n");
	for (i = 1; i <= 5; i++) {
		dccthread_sleep(ts);
		printf("%d\n", i);
	}
	dccthread_exit();
}
void test5(int dummy) {
	dccthread_t *t1 = dccthread_create("t1", f1, 0);
	dccthread_wait(t1);
	dccthread_exit();
}

int main(int argc, char **argv) {
	dccthread_init(test5, 0);
}
