#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "dccthread.h"

#define DCCTHREAD_MAX_NAME_SIZE 256
#define DCCTHREAD_STACK_SIZE 65536

typedef struct dccthread {
  int tid;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  ucontext_t context;
  int status;
  int blocking;
  struct dccthread *waiting;
  struct dccthread *next;
} dccthread_t;

int next_tid = 1;
dccthread_t *manager_c, *manager_p, *main_t;

dccthread_t * dccthread_new_thread (void) {
  dccthread_t *new = malloc(sizeof(dccthread_t));
  
  new->tid = next_tid++;
  new->context.uc_stack.ss_sp = NULL;
  new->context.uc_stack.ss_size = 0;
  new->context.uc_link = NULL;
  new->blocking = 0;
  new->waiting = NULL;
  getcontext(&new->context);
  
  return new;
}

void dccthread_init(void (*func)(int), int param) {
    main_t = dccthread_new_thread();
    
    main_t->next = main_t;
    manager_c = main_t;
    manager_p = main_t;
    
    main_t->context.uc_stack.ss_sp = malloc(DCCTHREAD_STACK_SIZE);
    main_t->context.uc_stack.ss_size = DCCTHREAD_STACK_SIZE;
    main_t->context.uc_stack.ss_flags = 0;
    strcpy(main_t->name, "main_t");
    
    makecontext(&main_t->context, func, 1, param);
    main_t->status = 1;
    
    setcontext(&main_t->context);
}

dccthread_t * dccthread_create(const char *name, void (*func)(int ), int param) {
    dccthread_t *new = dccthread_new_thread();
    
    new->context.uc_stack.ss_sp = malloc(DCCTHREAD_STACK_SIZE);
    new->context.uc_stack.ss_size = DCCTHREAD_STACK_SIZE;
    new->context.uc_stack.ss_flags = 0;
    //new->context.uc_link
    strcpy(new->name, name);
    
    makecontext(&new->context, func, 1, param);
    new->status = 0;
    
    new->next = manager_c;
    manager_p->next = new;
    manager_p = new;
}

void dccthread_yield(void) {
  if(manager_c == manager_p) return;
  
  manager_p = manager_c;
  manager_c = manager_c->next;
  
  while(manager_c->status == 2 && manager_p != NULL) manager_c = manager_c->next;
  
  manager_c->status = 1;
  swapcontext(&manager_p->context, &manager_c->context);
}

void dccthread_exit(void) {

}

void dccthread_wait(dccthread_t *tid) {

}

void dccthread_sleep(struct timespec ts) {

}

dccthread_t * dccthread_self(void) {
  return manager_c;
}

const char * dccthread_name(dccthread_t *tid) {
  return tid->name;
}