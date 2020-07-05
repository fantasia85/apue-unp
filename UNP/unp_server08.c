/* 
 * TCP预先创建线程服务器程序，主线程统一accept.
 * 对于主线程如何把一个已连接套接字传递给线程池中某个可用线程，可以使用描述符传递。既然所有线程和所有描述
 * 符都在同一个进程之内，就没有必要把一个描述符从一个进程传递到另一个线程。接受线程只需知道这个已连接套接
 * 字描述符的值。本例程中采用互斥锁和条件变量来保护所有线程共享的数据结构 
 * */

#include "tcpfunc.h"
#include <errno.h>

typedef struct {
    pthread_t thread_tid; /* thread ID */
    long thread_count; /* # connections handled */
} Thread;

Thread *tptr; /* array of Thread structures; calloc'ed */

#define MAXNCLI 32
int clifd[MAXNCLI], iget, iput; 
/* clifd数组由主线程往中存入已接受的已连接套接字描述符，并由线程池中的可用线程从中取出一个以服务相应的客户 */
pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER; 
/* 使用互斥锁和条件变量来保护线程共享的数据结构 */

static int nthreads;

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    void sig_int(int), thread_make(int);
    socklen_t addrlen, clilen;
    struct sockaddr *cliaddr;

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv08 [<host>] <port#> <#threads>");
    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    nthreads = atoi(argv[argc - 1]);
    if ((tptr = calloc(nthreads, sizeof(Thread))) == NULL)
        err_sys("calloc error");
    iget = iput = 0;

    /* create all the threads */
    for (i = 0; i < nthreads; i++)
        thread_make(i); /* only main thread returns */

    signal (SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
            err_sys("accept error");

        if (pthread_mutex_lock(&clifd_mutex) != 0)
            err_sys("pthread_mutex_lock error");
        clifd[iput] = connfd;
        if (++iput == MAXNCLI)
            iput = 0;
        if (iput == iget)
            err_quit("iput = iget = %d", iput);
        if (pthread_cond_signal(&clifd_cond) != 0)
            err_sys("pthread_cond_signal error");
        if (pthread_mutex_unlock(&clifd_mutex) != 0)
            err_sys("pthread_mutex_unlock error");
    }
}

void thread_make(int i)
{
    void *thread_main(void *);

    if (pthread_create(&tptr[i].thread_tid, NULL, &thread_main, (void *) i) != 0)
        err_sys("pthread_create error");

    return; /* main thread returns */
}

void *thread_main(void *arg)
{
    int connfd;
    void web_child(int);

    printf("thread %d starting\n", (int) arg);

    for ( ; ; ) {
        if (pthread_mutex_lock(&clifd_mutex) != 0)
            err_sys("pthread_mutex_lock error");
        while (iget == iput)
            if (pthread_cond_wait(&clifd_cond, &clifd_mutex) != 0)
                err_sys("pthread_cond_wait error");
        connfd = clifd[iget]; /* connected socket to service */
        if (++iget == MAXNCLI)
            iget = 0;
        if (pthread_mutex_unlock(&clifd_mutex) != 0)
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
