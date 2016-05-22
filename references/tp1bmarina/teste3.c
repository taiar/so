#include <stdlib.h>
#include <stdio.h>
#include "dccthread.h"

void f1() {
    unsigned long long i = 0;
    for(; i < 100000000ULL; i++);
    dccthread_exit();
}

void test2() {
    int i = 0;
    for(; i < 5; i++) {
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

