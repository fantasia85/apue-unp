/* TCP并发服务器程序，每个客户一个线程，以取代为每个客户派生一个子进程 */

#include "tcpfunc.h"
#include <errno.h>

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    void sig_int(int);
    void *doit(void *);
    pthread_t tid;
    socklen_t clilen, addrlen;
    struct sockaddr *cliaddr;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv06 [<host>] <port#>");
    
    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    signal(SIGINT, sig_int);

    for ( ; ; ) {
        clilen = addrlen;
        if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
            err_sys("accept error");

        if (pthread_create(&tid, NULL, &doit, (void *) connfd) != 0)
            err_sys("pthread_create error");
    }
}

void *doit(void *arg)
{
    void web_child(int);

    if (pthread_detach(pthread_self()) != 0)
        err_sys("pthread_detach error");

    web_child((int) arg);
    if (close((int) arg) < 0)
        err_sys("close error");
    
    return NULL;
}

void sig_int(int signo)
{
    void pr_cpu_time(void);

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