#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "dccthread.h"

#define DCCTHREAD_MAX_NAME_SIZE 256
#define DCCTHREAD_STACK_SIZE 65536
#define DCCTHREAD_MAX_THREADS 150

typedef struct dccthread {
  int id;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  ucontext_t context;
} dccthread_t;

dccthread_t *current_t, *manager_t, *main_t;
dccthread_t **threads;
int thread_number = 0;
int thread_current_id = 0;

void set_context_config(dccthread_t*);
void manager_function(void);

dccthread_t * dccthread_new(const char* name) {
  dccthread_t *new = malloc(sizeof(dccthread_t));
  strcpy(new->name, name);
  getcontext(&new->context);
  set_context_config(new);
  new->id = -1;
  
  return new;
}

void dccthread_init(void (*func)(void), int param) {
    manager_t = dccthread_new("MANAGER_THREAD");
    makecontext(&manager_t->context, manager_function, 0);

    main_t = dccthread_new("MAIN_THREAD");
    makecontext(&main_t->context, func, 1, param);

    threads = malloc(sizeof(dccthread_t*) * DCCTHREAD_MAX_THREADS);
    current_t = main_t;
    thread_current_id = -1;
    
    setcontext(&main_t->context);
}

dccthread_t * dccthread_create(const char *name, void (*func)(void), int param) {
    dccthread_t *new = dccthread_new(name);
    makecontext(&new->context, func, 1, param);
    new->id = thread_number;
    thread_number += 1;
    threads[new->id] = new;
    return new;
}

void dccthread_yield(void) {
  swapcontext(&current_t->context, &manager_t->context);
}

void dccthread_exit(void) {

}

void dccthread_wait(dccthread_t *id) {

}

void dccthread_sleep(struct timespec ts) {

}

dccthread_t * dccthread_self(void) {
  return current_t;
}

const char * dccthread_name(dccthread_t *id) {
  return id->name;
}

/***************************************/

void set_context_config(dccthread_t *thread) {
  thread->context.uc_link = 0;
  thread->context.uc_stack.ss_sp = malloc(SIGSTKSZ);
  thread->context.uc_stack.ss_size = SIGSTKSZ;
  thread->context.uc_stack.ss_flags = 0;
}

void manager_function() {
  thread_current_id = (thread_current_id + 1) % thread_number;
  current_t = threads[thread_current_id];
  swapcontext(&manager_t->context, &current_t->context);
}
