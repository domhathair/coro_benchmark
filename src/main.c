#define USE_MCO 1

#if USE_MCO
#define MINICORO_IMPL
#include "minicoro.h"
#else
#include "libco.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if USE_MCO
typedef mco_coro *thread_t;
#define thread_handler(n) void(n)(thread_t)
#define thread_yield() mco_yield(mco_running())
static int thread_create(thread_t *t, thread_handler(*th), size_t stack_size) {
    mco_desc desc = mco_desc_init(th, stack_size);
    mco_result res = mco_create(t, &desc);
    if (res != MCO_SUCCESS)
        return -1;

    return 0;
}
#define thread_resume mco_resume
#define thread_destroy mco_destroy
#else
typedef cothread_t thread_t;
#define thread_handler(n) void(n)(void)
#define thread_yield() co_switch(main_thread)
static int thread_create(thread_t *t, thread_handler(*th), size_t stack_size) {
    *t = co_create(stack_size, th);
    if (!t)
        return -1;

    return 0;
}
#define thread_resume co_switch
#define thread_destroy co_delete
#endif

static int64_t cur_time(void) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + (time.tv_nsec / 1000000);
}

static thread_t main_thread;

static size_t switches = 0;

#define create_thread(i)                                                                                               \
    static thread_handler(thread_##i) {                                                                                \
        while (1) {                                                                                                    \
            switches++;                                                                                                \
            thread_yield();                                                                                            \
        }                                                                                                              \
    }

create_thread(0);
create_thread(1);
create_thread(2);

static thread_handler(main_thread_handler) {
    static thread_t threads[3];
    thread_handler(*thread_handlers[3]) = {thread_0, thread_1, thread_2};

    for (size_t i = 0; i < 3; i++)
        if (thread_create(&threads[i], thread_handlers[i], 4096) != 0)
            return;

    int64_t ct = cur_time();
    while (1) {
        for (size_t i = 0; i < 3; i++) {
            thread_resume(threads[i]);
            if (cur_time() - ct >= 1)
                goto leave;
        }
    }
leave:
    printf("%zu\n", switches);

    for (size_t i = 0; i < 3; i++)
        thread_destroy(threads[i]);

    exit(0);
}

extern int main(int, char[]) {
    if (thread_create(&main_thread, main_thread_handler, 4096) != 0)
        return -1;

    thread_resume(main_thread);
    return 0;
}