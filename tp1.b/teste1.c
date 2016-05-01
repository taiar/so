#include <stdlib.h>
#include <stdio.h>

#include "dccthread.h"

void f1(int dummy) {
  dccthread_t *tid = dccthread_self();
  const char *name = dccthread_name(tid);
  int i = 0;
  for(i = 0; i < 3; i++) {
    printf("thread %s on iteration %d\n", name, i);
    dccthread_yield();
  }
}

void test1(int dummy) {
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
