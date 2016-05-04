#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define DCCTHREAD_MAX_NAME_SIZE 256

typedef struct dccthread {
  int tid;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  struct dccthread *next;
  ucontext_t context;
  int blocking;
} dccthread_t;

int next_tid = 1;
dccthread_t *current, *manager, *main_t;
struct sigevent t_event;
struct sigaction t_action, o_action;
struct itimerspec t_time;
timer_t timer;
timer_t* sleep_timer;

void dccthread_yield(void);
void context_init(ucontext_t*);

/*********************************************************/
//            API
void dccthread_init(void (*func)(void), int param) {
  dccthread_start_timer();
  main_t = dccthread_new_thread();
  
  main_t->next = main_t;
  current = main_t;
  manager = main_t;
  
  context_init(&main_t->context);
  
  strcpy(main_t->name, "main_t");
  
  makecontext(&main_t->context, func, 1, param);
  setcontext(&main_t->context);
}

dccthread_t * dccthread_create(const char *name, void (*func)(void), int param){
  dccthread_t *new = dccthread_new_thread();  
  context_init(&new->context);
  new->context.uc_link = &main_t->context;
  strcpy(new->name, name);
  
  makecontext(&new->context, func, 1, param);
  
  new->next = current;
  manager->next = new;
  manager = new;

  return new;
}

void dccthread_yield(void){
  if(current == manager) return;
  
  manager = current;
  current = current->next;
  
  swapcontext(&manager->context, &current->context);
}

void dccthread_exit(void){
  dccthread_t *end_t;
  
  if(current == manager){
    printf("No more threads to exit!\n");
    exit(0);
  }
  
  end_t = current;
  current = current->next;
  manager->next = current;
  
  printf("exiting %s\n", end_t->name);
  end_t->next = NULL;
  end_t->blocking = 0;
  dccthread_free(end_t);
  
  printf("setting context of %s\n", current->name);
  setcontext(&current->context);  
}

void dccthread_wait(dccthread_t *tid){
  if(tid == NULL) return;
  tid->blocking++;
  printf("%s esperando %s\n", current->name, tid->name);
  while(tid->next) dccthread_yield();
  tid->blocking--;
  printf("%s parou de esperar %s\n", current->name, tid->name);
}

void dccthread_sleep(struct timespec ts) {
  bloqueia_interrupcoes();
  dccthread_self()->soneca = ts.tv_sec * 1000000000 + ts.tv_nsec;
  dccthread_self()->status = DORMINDO;
  if (timerid_sleep == NULL) {
    inicializa_soneca();
  }
  clock_gettime(CLOCK_REALTIME, &ultima_thread_executada);

  gerencia_soneca();
  dccthread_yield();
}

dccthread_t * dccthread_self(void){
  return current;
}

const char * dccthread_name(dccthread_t *tid){
  return tid->name;
}

/*********************************************************/
//            TIMER

void dccthread_start_timer (void) {
  t_event.sigev_notify = SIGEV_SIGNAL;
  t_event.sigev_signo = SIGALRM;

  t_action.sa_handler = (void (*)())dccthread_yield;
  t_action.sa_flags  = 0;

  t_time.it_interval.tv_sec = t_time.it_value.tv_sec = 0;
  t_time.it_value.tv_nsec = t_time.it_interval.tv_nsec = 1000000;
  
  
  sigaction(SIGALRM, &t_action, &o_action);
  timer_create(CLOCK_PROCESS_CPUTIME_ID, &t_event, &timer);
  timer_settime(&timer, 0, &t_time, NULL);
}

/*********************************************************/
//             AUX

dccthread_t * dccthread_new_thread (void) {
  dccthread_t *new = malloc(sizeof(dccthread_t));
  
  new->tid = next_tid++;
  new->context.uc_stack.ss_sp = NULL;
  new->context.uc_stack.ss_size = 0;
  new->context.uc_link = NULL;
  new->blocking = 0;
  // new->waiting = NULL;
  
  getcontext(&new->context);
  
  return new;
}

void dccthread_free(dccthread_t *t){
  if(t->context.uc_stack.ss_sp != NULL) free(t->context.uc_stack.ss_sp);
  if(t->blocking == 0) free(t);
}

void context_init(ucontext_t* context) {
  context->uc_stack.ss_sp = malloc(SIGSTKSZ);
  context->uc_stack.ss_size = SIGSTKSZ;
  context->uc_stack.ss_flags = 0;
}