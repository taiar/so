#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

typedef enum {
    DCCTHREAD_NEW,
    DCCTHREAD_RUN,
    DCCTHREAD_END
} dccthread_state_t;


void dccthread_init(void (*func)(int), int param) {
  func(param);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int), int param);

void dccthread_yield(void);

void dccthread_exit(void);

void dccthread_wait(dccthread_t *tid);

void dccthread_sleep(struct timespec ts);

dccthread_t * dccthread_self(void);

const char * dccthread_name(dccthread_t *tid);
