#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>

typedef struct coro_t_		coro_t;
typedef struct thread_t_	thread_t;
typedef int (*coro_function_t)(coro_t *coro);
typedef enum {
    CORO_NEW,
    CORO_RUNNING,
    CORO_FINISHED
} coro_state_t;

#ifdef __x86_64__
union ptr_splitter {
    void *ptr;
    uint32_t part[sizeof(void *) / sizeof(uint32_t)];
};
#endif

struct thread_t_ {
    struct {
        ucontext_t callee, caller;
    } coro;
};

struct coro_t_ {
    coro_state_t state;
    coro_function_t function;
    thread_t *thread;

    ucontext_t context;
    char *stack;
    int yield_value;
};

static const int const default_stack_size = 4096;

void coro_yield(coro_t *coro, int value);

#ifdef __x86_64__
static void
_coro_entry_point(uint32_t part0, uint32_t part1)
{
    union ptr_splitter p;
    p.part[0] = part0;
    p.part[1] = part1;
    coro_t *coro = p.ptr;
    int return_value = coro->function(coro);
    coro->state = CORO_FINISHED;
    coro_yield(coro, return_value);
}
#else
static void
_coro_entry_point(coro_t *coro)
{
    int return_value = coro->function(coro);
    coro->state = CORO_FINISHED;
    coro_yield(coro, return_value);
}
#endif

coro_t *
coro_new(thread_t *thread, coro_function_t function)
{
    coro_t *coro = calloc(1, sizeof(*coro));

    coro->state = CORO_NEW;
    coro->stack = calloc(1, default_stack_size);
    coro->thread = thread;
    coro->function = function;

    getcontext(&coro->context);
    coro->context.uc_stack.ss_sp = coro->stack;
    coro->context.uc_stack.ss_size = default_stack_size;
    coro->context.uc_link = 0;

#ifdef __x86_64__
    union ptr_splitter p;
    p.ptr = coro;
    makecontext(&coro->context, (void (*)())_coro_entry_point, 2, p.part[0], p.part[1]);
#else
    makecontext(&coro->context, (void (*)())_coro_entry_point, 1, coro);
#endif

    return coro;
}

int
coro_resume(coro_t *coro)
{
    if (coro->state == CORO_NEW)
        coro->state = CORO_RUNNING;
    else if (coro->state == CORO_FINISHED)
        return 0;

    ucontext_t old_context = coro->thread->coro.caller;
    swapcontext(&coro->thread->coro.caller, &coro->context);
    coro->context = coro->thread->coro.callee;
    coro->thread->coro.caller = old_context;

    return coro->yield_value;
}

void
coro_yield(coro_t *coro, int value)
{
    coro->yield_value = value;
    swapcontext(&coro->thread->coro.callee, &coro->thread->coro.caller);
}

void
coro_free(coro_t *coro)
{
    free(coro->stack);
    free(coro);
}

int
g(coro_t *coro)
{
    int x = 0;
    puts("G: BEFORE YIELD, X = 0");
    x = 42;
    puts("G: BEFORE YIELD, X = 42");
    coro_yield(coro, 0);
    printf("G: AFTER YIELD, X = %d\n", x);

    return -1337;
}

int
f(coro_t *coro)
{
    coro_t *co2 = coro_new(coro->thread, g);
    coro_resume(co2);

    puts("F: BEFORE YIELD");
    coro_yield(coro, 42);
    puts("F: AFTER FIRST YIELD");
    coro_yield(coro, 1337);
    puts("F: AFTER SECOND YIELD");
    printf("F: g yielded %d\n", coro_resume(co2));
    coro_free(co2);

    return 0;
}

int
main(void)
{
    thread_t t;
    coro_t *co1 = coro_new(&t, f);
    printf("f yielded %d\n", coro_resume(co1));
    printf("f yielded %d\n", coro_resume(co1));
    printf("f yielded %d\n", coro_resume(co1));
    puts("DONE");
    coro_free(co1);
    return 0;
}