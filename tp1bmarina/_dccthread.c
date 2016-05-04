#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>

#define DCCTHREAD_MAX_NAME_SIZE 256

typedef enum {READY, RUNNING, WAITING, ENDED} ThreadStatus; 

typedef struct dccthread {
  int tid;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  struct dccthread *next;
  ucontext_t context;
  ThreadStatus status;
  int blocking;
  struct dccthread *waiting;  
} dccthread_t;

int next_tid = 1;
dccthread_t *manager_c, *manager_p, *main_t;

void context_init(ucontext_t*);

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

void dccthread_init(void (*func)(void), int param){
  main_t = dccthread_new_thread();
  
  main_t->next = main_t;
  manager_c = main_t;
  manager_p = main_t;
  
  context_init(&main_t->context);
  
  strcpy(main_t->name, "main_t");
  main_t->status = RUNNING;
  
  makecontext(&main_t->context, func, 1, param);
  setcontext(&main_t->context);
}

dccthread_t * dccthread_create(const char *name, void (*func)(void), int param){
  dccthread_t *new = dccthread_new_thread();  
  context_init(&new->context);
  new->context.uc_link = &main_t->context;
  strcpy(new->name, name);
  new->status = READY;
  
  makecontext(&new->context, func, 1, param);
  
  new->next = manager_c;
  manager_p->next = new;
  manager_p = new;

  return new;
}

void dccthread_free(dccthread_t *t){
  if(t->context.uc_stack.ss_sp != NULL) free(t->context.uc_stack.ss_sp);
  if(t->blocking == 0) free(t);
}

void dccthread_yield(void){
  if(manager_c == manager_p) return;
  
  manager_p = manager_c;
  manager_c = manager_c->next;
  
  while(manager_c->status == WAITING && manager_p != NULL)
    manager_c = manager_c->next;

  manager_c->status = RUNNING;
  swapcontext(&manager_p->context, &manager_c->context);
}

void dccthread_exit(void){
  dccthread_t *end_t;
  
  if(manager_c == manager_p){
    printf("No more threads to exit!\n");
    exit(0);
  }
  
  end_t = manager_c;
  manager_c = manager_c->waiting;
  manager_c->next = end_t->next;
  manager_p->next = manager_c;
  
  printf("exiting %s\n", end_t->name);
  end_t->next = NULL;
  end_t->status = ENDED;
  dccthread_free(end_t);
  
  printf("setting context of %s\n", manager_c->name);
  setcontext(&manager_c->context);  
}

void dccthread_wait(dccthread_t *tid){
  if(tid == NULL) return;
  tid->blocking++;
  tid->waiting = manager_c;
  manager_c->status = WAITING;
  printf("%s esperando %s\n", manager_c->name, tid->name);
  while(tid->next != NULL) dccthread_yield();
  manager_c->status = RUNNING;
  printf("%s parou de esperar %s\n", manager_c->name, tid->name);
}

void dccthread_sleep(struct timespec ts){}

dccthread_t * dccthread_self(void){
  return manager_c;
}

const char * dccthread_name(dccthread_t *tid){
  return tid->name;
}

/*********************************************************/

void context_init(ucontext_t* context) {
  context->uc_stack.ss_sp = malloc(SIGSTKSZ);
  context->uc_stack.ss_size = SIGSTKSZ;
  context->uc_stack.ss_flags = 0;
}