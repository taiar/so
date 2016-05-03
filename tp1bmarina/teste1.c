#include <stdlib.h>
#include <stdio.h>

#include "dccthread.h"

void f1(void) {
  dccthread_t *tid = dccthread_self();
  const char *name = dccthread_name(tid);
  int i = 0;
  for(i = 0; i < 3; i++) {
    printf("thread %s on iteration %d\n", name, i);
    dccthread_yield();
  }
}

void test2(void) {
  int i = 0;
  for(; i < 5; i++) {
    dccthread_t *t1 = dccthread_create("t1", f1, 0);
    dccthread_t *t2 = dccthread_create("t2", f1, 0);
    dccthread_wait(t1);
    dccthread_wait(t2);
  }
  dccthread_exit();
}

void test1(void) {
  int i = 0;
  dccthread_create("t1", f1, 0);
  dccthread_create("t2", f1, 0);
  dccthread_create("t3", f1, 0);
  for(i = 0; i < 10; i++) {
    printf("test1 yielding\n");
    dccthread_yield();
  }
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  dccthread_init(test1, 0);
  return 0;
}
