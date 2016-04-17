#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>

#include "dccthread.h"

// TODO: verificar se tem que existir mesmo um número máximo definido 
#ifndef MAX_THREAD
#define MAX_THREADS 999
#endif

typedef enum {
    DCCTHREAD_NEW,
    DCCTHREAD_RUN,
    DCCTHREAD_END
} dccthread_state;

struct dccthread {
  int id;
  ucontext_t context;
  char* name;
  dccthread_state status;
};

struct thread_state {

};


void manage_threads();

// Vetor de threads do sistema
static struct dccthread *threads[MAX_THREADS];

// Primeirésima thread
static struct dccthread *main_thread = NULL;

// Thread gerente
static struct dccthread *manager_thread = NULL;

// Thread corrente
static struct dccthread *current_thread = NULL;

static int current_thread_id = 0;

static int thread_number = 0;

void dccthread_init(void (*func)(int), int param) {
  // threads = malloc(sizeof(dccthread_t*) * MAX_THREADS);
  memset(threads, 0, sizeof(dccthread_t*) * MAX_THREADS);

  manager_thread = NULL;
  dccthread_create("MANAGER_THREAD", &manage_threads, 0);
  makecontext(&(manager_thread->context), manage_threads, 0);

  struct dccthread * main_thread = NULL;
  dccthread_create("MAIN_THREAD", func, param);
  makecontext(&(main_thread->context), NULL, param);

  current_thread = main_thread;
  threads[0] = main_thread;
  thread_number = 1;
  current_thread_id = main_thread->id;
  setcontext(&(main_thread->context));
}

dccthread_t* dccthread_create(const char *name, void (*func)(int), int param) {
  dccthread_t* thread = malloc(sizeof(dccthread_t));
  getcontext(&(thread)->context);
  thread->name = malloc(sizeof(char) * (strlen(name) + 1));
  strcpy(thread->name, name);
  thread->status = DCCTHREAD_NEW;
  return thread;
}

void dccthread_yield(void) {
  swapcontext(&(current_thread->context), &(manager_thread->context));
}

void dccthread_exit(void);

void dccthread_wait(dccthread_t *tid);

void dccthread_sleep(struct timespec ts);

dccthread_t * dccthread_self(void) {
  return main_thread;
}

const char * dccthread_name(dccthread_t *tid) {
  return tid->name;
}

//*********************
//*********************

void manage_threads(void) {
  while (thread_number != 0) {
    current_thread_id = (current_thread_id + 1) % thread_number;
    while ((threads[current_thread_id])->status != DCCTHREAD_END) {
      current_thread_id = (current_thread_id + 1) % thread_number;
    }
    current_thread = threads[current_thread_id];
    swapcontext(&(manager_thread->context), &(threads[current_thread_id]->context));
  }
  manager_thread->status = DCCTHREAD_END;
  // free thread memory
}