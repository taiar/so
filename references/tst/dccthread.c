#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <string.h>

#define DCCTHREAD_MAX_NAME_SIZE 256
#define DCCTHREAD_STACK_SIZE 65536

 typedef struct dccthread {
   
  int tid;
  char name[DCCTHREAD_MAX_NAME_SIZE];
  struct dccthread *next;
  ucontext_t context;
  int status; // 0 ready 1 running 2 waiting 3 ended
  int blocking;
  struct dccthread *waiting;
  
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

/* `dccthread_init` initializes any state necessary for the
 * threadling library and starts running `func`.  this function
 * never returns. */
void dccthread_init(void (*func)(int), int param){
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

/* on success, `dccthread_create` allocates and returns a thread
 * handle.  returns `NULL` on failure.  the new thread will execute
 * function `func` with parameter `param`.  `name` will be used to
 * identify the new thread. */
dccthread_t * dccthread_create(const char *name, void (*func)(int), int param){
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

void dccthread_free(dccthread_t *t){
  if(t->context.uc_stack.ss_sp != NULL) free(t->context.uc_stack.ss_sp);
  if(t->blocking == 0) free(t);
  t = NULL;
}

/* `dccthread_yield` will yield the CPU (from the current thread to
 * another). */
void dccthread_yield(void){
  if(manager_c == manager_p) return;
  
  manager_p = manager_c;
  manager_c = manager_c->next;
  
  while(manager_c->status == 2 && manager_p != NULL) manager_c = manager_c->next;
  
  manager_c->status = 1;
  printf("trocando contexto de %s por %s\n\n", manager_p->name, manager_c->name);
  swapcontext(&manager_p->context, &manager_c->context);
}

/* `dccthread_exit` terminates the current thread, freeing all
 * associated resources. */
void dccthread_exit(void){
  dccthread_t *end_t;
  
  if(manager_c == manager_p){
    printf("No more threads to exit!\n");
    exit(0);
  }
  
  end_t = manager_c;
  manager_c = manager_c->next;
  manager_p->next = manager_c;
  
  printf("dando exit em %s\n\n", end_t->name);
  end_t->status = 3;
  end_t->next = NULL;
  dccthread_free(end_t);
  
  printf("setando contexto de %s\n\n", manager_c->name);
  setcontext(&manager_c->context);  
}

/* `dccthread_wait` blocks the current thread until thread `tid`
 * terminates. */
void dccthread_wait(dccthread_t *tid){
  tid->blocking++;
  manager_c->status = 2;
  tid->waiting = manager_c;
  printf("%s esperando %s\n\n", manager_c->name, tid->name);
  while(tid->next != NULL) dccthread_yield();
  printf("%s parou de esperar %s\n\n", manager_c->name, tid->name);
  manager_c->status = 1;
  tid->blocking--;
  dccthread_free(tid);
}

/* `dccthread_sleep` stops the current thread for the time period
 * specified in `ts`. */
void dccthread_sleep(struct timespec ts){

}

/* `dccthread_self` returns the current thread's handle. */
dccthread_t * dccthread_self(void){
  return manager_c;
}

/* `dccthread_name` returns a pointer to the string containing the
 * name of thread `tid`.  the returned string is owned and managed
 * by the library. */
const char * dccthread_name(dccthread_t *tid){
  return tid->name;
}

void f1(void *data) {
  dccthread_t *tid = dccthread_self();
  char *name = dccthread_name(tid);
  for(int i = 0; i < 3; i++) {
    printf("thread %s on iteration %d\n", name, i);
    dccthread_yield();
  }
  dccthread_exit();
}

void test2(int dummy) {
  for(int i = 0; i < 5; i++) {
    dccthread_t *t1 = dccthread_create("t1", f1, 0);
    dccthread_t *t2 = dccthread_create("t2", f1, 0);
    dccthread_wait(t1);
    dccthread_wait(t2);
  }
  dccthread_exit();
}

int main(int argc, char **argv) {
  dccthread_init(test1, 0);
}
