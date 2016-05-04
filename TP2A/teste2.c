#include <stdlib.h>
#include <stdio.h>

#include "dccthread.h"

void f1(void *data) {
	dccthread_t *tid = dccthread_self();
	char *name = dccthread_name(tid);
	int i;
	for (i = 0; i < 3; i++) {
		printf("thread %s on iteration %d\n", name, i);
		dccthread_yield();
	}
	dccthread_exit();
}

void test2(int dummy) {
	int i;
	for (i = 0; i < 5; i++) {
		dccthread_t *t1 = dccthread_create("t1", f1, 0);
		dccthread_t *t2 = dccthread_create("t2", f1, 0);
		dccthread_wait(t1);
		dccthread_wait(t2);
	}
	dccthread_exit();
}

int main(int argc, char **argv) {
	dccthread_init(test2, 0);
}
