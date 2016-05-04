#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#include "dccthread.h"

void inicializa_pilha(ucontext_t * context);
void cria_thread(int id, const char* name, struct dccthread **thread);
void gerencia_threads();
int thread_pronta(struct dccthread *thread);
void remove_thread(int id);
void destroi_thread(struct dccthread *thread);
void bloqueia_interrupcoes();
void desbloqueia_interrupcoes();
void inicializa_timer();
timer_t create_timer(struct sigevent sig_event, int signo, clockid_t CLK_id);
void install_sighandler(struct sigaction sig_action, int signo,
		void (*handler)(int));
void set_timer(struct itimerspec timervals, timer_t timerid, int sec, int nsec,
		char periodically);
void inicializa_soneca();
void gerencia_soneca(void);
void atualiza_timer();

#define TRUE 1
#define FALSE 0

#define PRONTA 0
#define ESPERANDO 1
#define DORMINDO 2
#define ENCERRADA 3

#define MAX_NUM_THREADS 20

int numThreads = 0;         // numero de threads executando
int idThreadRodando = 0;    // identificador da thread que esta rodando

struct dccthread {
	ucontext_t context;    // estrutura que representa o contexto de cada thread
	int id;		            // identificador da thread
	char* name;	            // nome da thread
	int status;	            // status da thread
							// Ela pode estar: pronta, esperando, dormindo ou encerrada 
	dccthread_t *esperando; // Ponteiro para a thread que está esperando
	long soneca;	        // indice que indica se a thread esta dormindo
};

struct dccthread * threadGerente = NULL;
struct dccthread * threadCorrente = NULL;
struct dccthread * threads[MAX_NUM_THREADS];

timer_t timerid;
timer_t* timerid_sleep;

struct timespec ultima_thread_executada;
struct itimerspec its_sleep;

/* Aloca a pilha para o contexto do programa */
void inicializa_pilha(ucontext_t * context) {
	context->uc_link = 0;
	context->uc_stack.ss_sp = malloc(SIGSTKSZ);
	context->uc_stack.ss_size = SIGSTKSZ;
	context->uc_stack.ss_flags = 0;
}

/* Cria a estrutura de uma thread, quando o dccthread_init eh chamado */
void cria_thread(int id, const char* name, struct dccthread **thread) {
	if (*thread != NULL) {
		destroi_thread(*thread);
	}
	(*thread) = malloc(sizeof(dccthread_t));
	getcontext(&(*thread)->context);
	inicializa_pilha(&((*thread)->context));

	(*thread)->id = id;
	int len = strlen(name) + 1;
	(*thread)->name = malloc(sizeof(char) * len);
	strcpy((*thread)->name, name);
	(*thread)->status = PRONTA;
	(*thread)->esperando = FALSE;
	(*thread)->soneca = FALSE;
}

/* Funcao gerenciadora das threads. 
 * Ela escolhe qual sera a proxima thread a executar */
void gerencia_threads() {
	while (numThreads != 0) {
		bloqueia_interrupcoes();
		idThreadRodando = (idThreadRodando + 1) % numThreads;
		while (!thread_pronta(threads[idThreadRodando])) {
			idThreadRodando = (idThreadRodando + 1) % numThreads;
		}
		threadCorrente = threads[idThreadRodando];
		desbloqueia_interrupcoes();
		swapcontext(&(threadGerente->context),
				&(threads[idThreadRodando]->context));
	}
	bloqueia_interrupcoes();
	threadGerente->status = ENCERRADA;
	destroi_thread(threadGerente);
}

/* Analisa se uma thread esta pronta pra executar */
int thread_pronta(struct dccthread *thread) {
	if (thread == NULL) {
		return FALSE;
	} else {
		return (thread->status == PRONTA);
	}
}

/* Funcao que remove uma thread, quando o dccthread_exit eh chamado */
void remove_thread(int id) {
	destroi_thread(threads[id]);
	numThreads--;
	threads[id] = NULL;
	threads[id] = threads[numThreads];
	if (id != numThreads) {
		threads[id]->id = id;
	}
	threads[numThreads] = NULL;
}

/* Libera a parte da pilha usada no contexto do programa */
void destroi_thread(struct dccthread *thread) {
	free(thread->context.uc_stack.ss_sp);
	free(thread->name);
}

/* Bloqueia interrupcoes que podem ser geradas pelo sistema 
 * em certa parte do codigo */
void bloqueia_interrupcoes() {
	sigset_t sinal;
	sigaddset(&sinal, SIGRTMIN);
	sigprocmask(SIG_BLOCK, &sinal, NULL);
}

/* Desbloqueia interrupcoes */
void desbloqueia_interrupcoes() {
	sigset_t sinal;
	sigaddset(&sinal, SIGRTMIN);
	timer_delete(timerid);
	inicializa_timer();
	sigprocmask(SIG_UNBLOCK, &sinal, NULL);
}

/* Inicializa o temporizador, que sera chamado regularmente, 
 * para trocar a thread que esta executando */
void inicializa_timer() {
	long freq_microsecs = 10;

	struct sigevent sig_event;
	timerid = create_timer(sig_event, SIGRTMIN, CLOCK_PROCESS_CPUTIME_ID);

	struct sigaction sig_action;
	install_sighandler(sig_action, SIGRTMIN, (void (*)()) dccthread_yield);

	struct itimerspec timervals;
	set_timer(timervals, timerid, freq_microsecs / 1000000,
			freq_microsecs % 1000000, 1);
}

/* Cria o temporizador */
timer_t create_timer(struct sigevent sig_event, int signo, clockid_t CLK_id) {
	timer_t tid;
	sig_event.sigev_notify = SIGEV_SIGNAL;
	sig_event.sigev_signo = signo;
	if (timer_create(CLK_id, &sig_event, &tid) == -1) {
		perror("Failed to create timer");
		exit(-1);
	}
	return tid;
}

/* Estabelece manipulacao para o temporizador dos sinais */
void install_sighandler(struct sigaction sig_action, int signo,
		void (*handler)(int)) {
	sig_action.sa_handler = handler;
	sig_action.sa_flags = SA_SIGINFO;
	sigaction(signo, &sig_action, NULL);
}

/* Dispara o temporizador */
void set_timer(struct itimerspec timervals, timer_t timerid, int sec, int nsec,
		char periodically) {
	timervals.it_value.tv_sec = sec;
	timervals.it_value.tv_nsec = nsec;
	if (periodically) {
		timervals.it_interval.tv_sec = sec;
		timervals.it_interval.tv_nsec = nsec;
	} else {
		timervals.it_interval.tv_sec = 0;
		timervals.it_interval.tv_nsec = 0;
	}
	timer_settime(&timerid, 0, &timervals, NULL);
}

/* Inicializa o temporizador, que sera usado quando dccthread_sleep for chamada */
void inicializa_soneca() {
	timerid_sleep = malloc(sizeof(timer_t));

	struct sigevent sig_event_sleep;
	timerid_sleep = create_timer(sig_event_sleep, SIGRTMAX, CLOCK_REALTIME);

	struct sigaction sig_action_sleep;
	install_sighandler(sig_action_sleep, SIGRTMAX,
			(void (*)()) gerencia_soneca);
}

/* Funcao gerenciadora da soneca. Ela escolhe qual sera a proxima thread 
 * a executar */
void gerencia_soneca(void) {
	bloqueia_interrupcoes();
	struct timespec times;
	clock_gettime(CLOCK_REALTIME, &times);
	long tempoGasto = (times.tv_sec - ultima_thread_executada.tv_sec)
			* 1000000000 + (times.tv_nsec - ultima_thread_executada.tv_nsec);
	int i;
	for (i = 0; i < numThreads; i++) {
		threads[i]->soneca -= tempoGasto;
		if (threads[i]->soneca <= 0 && threads[i]->status == DORMINDO) {
			threads[i]->soneca = 0;
			threads[i]->status = PRONTA;
		}
	}
	atualiza_timer();
	desbloqueia_interrupcoes();
}

/* Atualiza o temporizador. Eh usada para controlar o timer quando 
 * dccthread_sleep eh chamada */
void atualiza_timer() {
	long freq_soneca = LONG_MAX;
	int i;
	for (i = 0; i < numThreads; i++) {
		if (threads[i]->status == DORMINDO
				&& threads[i]->soneca < freq_soneca) {
			freq_soneca = threads[i]->soneca;
		}
	}
	if (freq_soneca == LONG_MAX) {
		freq_soneca = 0;
	}

	its_sleep.it_value.tv_sec = freq_soneca / 1000000000;
	its_sleep.it_value.tv_nsec = freq_soneca % 1000000000;
	its_sleep.it_interval.tv_sec = freq_soneca / 1000000000;
	its_sleep.it_interval.tv_nsec = freq_soneca % 1000000000;

	timer_settime(timerid_sleep, 0, &its_sleep, NULL);
}

/* Inicializa a thread gerente e a thread principal, alem de executar a principal */
void dccthread_init(void (*func)(int), int param) {
	inicializa_timer();
	memset(threads, 0, sizeof(dccthread_t*) * MAX_NUM_THREADS);

	threadGerente = NULL;
	cria_thread(-1, "Thread gerente", &threadGerente);
	makecontext(&(threadGerente->context), gerencia_threads, 0);

	struct dccthread * threadPrincipal = NULL;
	cria_thread(0, "Thread principal", &threadPrincipal);
	makecontext(&(threadPrincipal->context), func, 1, param);

	threadCorrente = threadPrincipal;
	threads[0] = threadPrincipal;
	numThreads = 1;
	idThreadRodando = threadPrincipal->id;
	setcontext(&(threadPrincipal->context));
}

/* Cria uma thread */
struct dccthread * dccthread_create(const char *name, void (*func)(), int param) {
	bloqueia_interrupcoes();
	int i;
	struct dccthread * thread = NULL;
	cria_thread(numThreads, name, &thread);
	threads[numThreads++] = thread;
	makecontext(&(thread->context), func, 1, param);
	desbloqueia_interrupcoes();

	return thread;
}

/* Realiza a troca de contexto entre a thread que esta executando e a gerente */
void dccthread_yield(void) {
	bloqueia_interrupcoes();
	swapcontext(&(threadCorrente->context), &(threadGerente->context));
	desbloqueia_interrupcoes();
}

/* Remove a thread que esta executando */
void dccthread_exit(void) {
	bloqueia_interrupcoes();
	if (dccthread_self()->status != ENCERRADA) {
		dccthread_self()->status = ENCERRADA;
		int i;
		for (i = 0; i < numThreads; i++) {
			if (threads[i]->status == ESPERANDO
					&& threads[i]->esperando == threadCorrente) {
				threads[i]->status = PRONTA;
				threads[i]->esperando = NULL;
			}
		}
		remove_thread(dccthread_self()->id);
		dccthread_yield();
	} else {
		fprintf(stderr, "Terminating already terminated thread%d @[%s;%d]\n",
				idThreadRodando, __FILE__, __LINE__);
	}
	desbloqueia_interrupcoes();
}

/* Suspende a execucao de uma thread enquando outra nao termina a sua execucao */
void dccthread_wait(dccthread_t *tid) {
	bloqueia_interrupcoes();
	if (tid->status == ENCERRADA) {
		desbloqueia_interrupcoes();
		return;
	}
	if (dccthread_self()->status == PRONTA) {
		dccthread_self()->status = ESPERANDO;
		dccthread_self()->esperando = tid;
	}
	dccthread_yield();
}

/* Faz com que a thread que esta executando durma por um tempo */
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

/* Retorna a thread que esta executando */
dccthread_t * dccthread_self() {
	return threads[idThreadRodando];
}

/* Retorna o nome de uma thread */
const char * dccthread_name(struct dccthread *tid) {
	return tid->name;
}

