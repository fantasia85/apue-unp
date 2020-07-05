/* TCP预先创建线程服务器程序，每个线程各自accept 
 * 预先创建一个线程池，并让每个线程各自调用accept，取代每个线程都阻塞在accept调用之中的做法，并用互斥锁
 * 保证任何时刻只有一个线程在调用accept */

#include "tcpfunc.h"
#include <errno.h>

typedef struct {
    pthread_t thread_tid; /* thread ID */
    long thread_count; /* # connections handled */
}Thread;

Thread *tptr; /* array of Thread structures; calloc'ed */

int listenfd, nthreads;
socklen_t addrlen;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int i;
    void sig_int(int), thread_make(int);

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv07 [<host>] <port#> <#threads>");

    nthreads = atoi(argv[argc -1]);
    if ((tptr = calloc(nthreads, sizeof(Thread))) == NULL)
        err_sys("calloc error");

    for (i = 0; i < nthreads; i++)
        thread_make(i); /* only main thread returns */

    signal(SIGINT, sig_int);

    for ( ; ; )
        pause(); /* everything done by threads */
}

void thread_make(int i)
{
    void *thread_main(void *);

    if (pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i) != 0)
        err_sys("pthread_create error");
    
    return; /* main thread returns */
}

void *thread_main(void * arg)
{
    int connfd;
    void web_child(int);
    socklen_t clilen;
    struct sockaddr *cliaddr;

    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    printf("thread %d starting\n", (int) arg);
    for ( ; ; ) {
        clilen = addrlen;
        
        if (pthread_mutex_lock(&mlock) != 0)
            err_sys ("pthread_mutex_lock error");
        if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
            err_sys("accept error");
        if (pthread_mutex_unlock(&mlock) != 0)
            err_sys("pthread_mutex_unlock error");

        tptr[(int) arg].thread_count++;

        web_child(connfd); /* process request */
        if (close(connfd) < 0)
            err_sys("close error");
    }
}

void sig_int(int signo)
{
    void pr_cpu_time(void);

    printf("\n");
    for (int i = 0; i < nthreads; i++)
        printf("thread %ld send %d times\n", tptr[i].thread_tid, tptr[i].thread_count);

    pr_cpu_time();
    exit(0);
}

#include <sys/resource.h>

#ifndef HAVE_GETRUSAGE_PROTO
int getrusage(int, struct rusage *);
#endif

void pr_cpu_time(void)
{
    double user, sys;
    struct rusage myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
        err_sys("getrusage error");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        err_sys("getrusage error");

    user = (double) myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec / 1000000.0;
    user += (double) childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec / 1000000.0;
    sys = (double) myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec / 1000000.0;
    sys += (double) childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec / 1000000.0;

    printf("\nuser time = %g, sys time = %g\n", user, sys);
}

#define MAXN 16384 /* max # bytes client can request */

void web_child(int sockfd)
{
    int ntowrite, n;
    ssize_t nread;
    char line[MAXLINE], result[MAXLINE];

    int nreply = 4000;
    snprintf(result, sizeof(result), "%d\n", nreply);

    for ( ; ; ) {
        if ((nread = read(sockfd, line, MAXLINE)) < 0)
            err_sys("read error");
        else if (nread == 0)
            return; /* connection closed by other end */
        
        /* line from client specifies # bytes to write back */
        ntowrite = atol(line);
        if ((ntowrite <= 0) || (ntowrite > MAXN))
            err_quit("client request for %d bytes", ntowrite);

        if ((n = writen(sockfd, result, ntowrite)) < 0)
            err_sys("writen error");
    }
}