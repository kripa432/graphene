#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static int counter = 0;

static void handler(int signum) {
    __atomic_add_fetch(&counter, 1, __ATOMIC_SEQ_CST);
}

static int sync_var = 0;

static void set(int x) {
    __atomic_store_n(&sync_var, x, __ATOMIC_SEQ_CST);
}

static void wait_for(int x) {
    while (__atomic_load_n(&sync_var, __ATOMIC_SEQ_CST) != x) {
        sched_yield();
    }
}

static void* f(void* x) {
    set(1);
    wait_for(2);
    return x;
}

int main() {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    struct sigaction action = { 0 };
    action.sa_handler = handler;

    int ret = sigaction(SIGTERM, &action, NULL);
    if (ret < 0) {
        fprintf(stderr, "sigaction failed\n");
        return 1;
    }

    pthread_t th;

    if (pthread_create(&th, NULL, f, NULL)) {
        fprintf(stderr, "pthread_create failed: %m\n");
        return 1;
    }

    wait_for(1);

    if (kill(getpid(), SIGTERM)) {
        fprintf(stderr, "kill failed: %m\n");
        return 1;
    }

    /* Poor man's way of allowing the other thread to handle the signal, if it was delivered to it.
     * That thread calls sched_yield in a loop, so 1ms should be enough. */
    usleep(1000);

    set(2);

    if (pthread_join(th, NULL)) {
        fprintf(stderr, "pthread_join failed: %m\n");
        return 1;
    }

    int t = __atomic_load_n(&counter, __ATOMIC_SEQ_CST);
    if (t != 1) {
        fprintf(stderr, "test failed: handler was run %d times\n", t);
        return 1;
    }

    puts("TEST OK!");

    return 0;
}