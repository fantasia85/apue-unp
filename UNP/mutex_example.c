//互斥锁mutex的使用例程

#include "apue.h"

#define NLOOP 5000

int counter; /* incremented by threads */
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

int log_to_stderr = 1;

void *doit(void *);

int main(int argc, char **argv)
{
    pthread_t tida, tidb;

    if (pthread_create(&tida, NULL, &doit, NULL) != 0)
        err_sys("pthread_create a error");
    if (pthread_create(&tidb, NULL, &doit, NULL) != 0)
        err_sys("pthread_create b error");

    /* wait for both threads to terminate */
    if (pthread_join(tida, NULL) != 0)
        err_sys("pthread_join a error");
    if (pthread_join(tidb, NULL) != 0)
        err_sys("pthread_join b error");

    exit(0);
}

void *doit(void *vptr)
{
    int i, val;

    /* Each thread fetches, prints, and increments the counter NLOOP times.
     * The value of the counter should increase monotonically. 
     */

    for (i = 0; i < NLOOP; i++) {
        if (pthread_mutex_lock(&counter_mutex) != 0)
            err_sys("pthread_mutex_lock error");

        val = counter;
        printf("%d: %d\n", pthread_self(), val + 1);
        counter = val + 1;

        if (pthread_mutex_unlock(&counter_mutex) != 0)
            err_sys("pthread_mutex_unlock error");
    }

    return NULL;
}