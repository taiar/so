#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "dccthread.h"

#define DCCTHREAD_MAX_NAME_SIZE 256
#define DCCTHREAD_STACK_SIZE 65536
#define DCCTHREAD_MAX_THREADS 999

typedef struct dccthread {
  int id;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  ucontext_t context;
  int status;
  int blocking;
  struct dccthread *waiting;
  struct dccthread *next;
} dccthread_t;

dccthread_t *current_t, *manager_t, *main_t;
dccthread_t *threads[DCCTHREAD_MAX_THREADS];
int thread_number = 0;
int thread_current_id = 0;

void set_context_config(dccthread_t*);
void manager_function(void);

dccthread_t * dccthread_new(void) {
  dccthread_t *new = malloc(sizeof(dccthread_t));
  new->context.uc_stack.ss_sp = NULL;
  new->context.uc_stack.ss_size = 0;
  new->context.uc_link = NULL;
  new->id = -1;
  new->blocking = 0;
  new->waiting = NULL;
  getcontext(&new->context);
  
  return new;
}

void dccthread_init(void (*func)(int), int param) {
    manager_t = dccthread_new();
    set_context_config(manager_t);
    strcpy(manager_t->name, "MANAGER_THREAD");
    makecontext(&manager_t->context, manager_function, 1, 0);
    manager_t->id = thread_current_id;
    manager_t->status = 0;

    main_t = dccthread_new();
    set_context_config(main_t);
    strcpy(main_t->name, "MAIN_THREAD");
    makecontext(&main_t->context, func, 1, param);
    thread_current_id += 1;
    main_t->id = thread_current_id;
    main_t->status = 1;
    
    thread_number = 2;
    memset(threads, 0, sizeof(dccthread_t*) * DCCTHREAD_MAX_THREADS);
    threads[0] = manager_t;
    threads[1] = main_t;
    current_t = main_t;
    thread_current_id = 1;

    setcontext(&current_t->context);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int), int param) {
    dccthread_t *new = dccthread_new();
    set_context_config(new);
    strcpy(new->name, name);
    makecontext(&new->context, func, 1, param);
    new->status = 0;
    new->id = thread_number;
    thread_number += 1;
    threads[new->id] = new;
}

void dccthread_yield(void) {
  printf("YELDIN\n");
  swapcontext(&(current_t->context), &(manager_t->context));
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
  thread->context.uc_stack.ss_sp = malloc(DCCTHREAD_STACK_SIZE);
  thread->context.uc_stack.ss_size = DCCTHREAD_STACK_SIZE;
  thread->context.uc_stack.ss_flags = 0;
}

void manager_function() { 
  printf("MANAGIN\n");
  int next_thread_idx;
  if(thread_current_id <= 1) {
    next_thread_idx = 2;
  } else {
    if((thread_current_id + 1) == thread_number)
      next_thread_idx = 2;
    else 
      next_thread_idx = thread_current_id + 1;
  }
  thread_current_id = next_thread_idx;
  current_t = threads[next_thread_idx];
  swapcontext(&(manager_t->context), &(threads[next_thread_idx]->context));
}
