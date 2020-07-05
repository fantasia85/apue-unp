//TCP预先派生子进程服务器程序，accept无上锁保护
/* 使用预先派生子进程(preforking)的技术。使用该技术的服务器不像传统意义的并发服务器那样为每个客户现场派生一个子进程，
 * 而是在启动阶段预先派生一定数量的子进程，当每个客户连接到达时，这些子进程立即就能够为它们服务。 */

#include "tcpfunc.h"
#include <errno.h>
#include <sys/wait.h>

static int nchildren;  /* 预先派生的子进程数量 */
static pid_t *pids; /* 用以保留各子进程pid的数组 */

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, i;
    socklen_t addrlen;
    void sig_int(int);
    pid_t child_make(int, int, int); /* 创建各个子进程 */

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv02 [<host>] <port#> <#children>");

    nchildren = atoi(argv[argc - 1]); //预先派生的子进程个数
    if ((pids = calloc(nchildren, sizeof(pid_t))) == NULL)
        err_sys("calloc error"); //为每个子进程的pid分配空间

    for (i = 0; i < nchildren; i++)
        pids[i] = child_make(i, listenfd, addrlen); /* parent returns */
    
    signal(SIGINT, sig_int);

    for ( ; ; )
        pause(); /* everything done by children */
}


void sig_int(int signo)
{
    int i;
    void pr_cpu_time(void);

    /* terminate all children */
    for (i = 0; i < nchildren; i++)
        kill(pids[i], SIGTERM);
    while (wait(NULL) > 0)
        ;  /* wait for all children */
    if (errno != ECHILD)
        err_sys("wait error");

    pr_cpu_time();
    exit (0);
}

#include <sys/resource.h>

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

pid_t child_make(int i, int listenfd, int addrlen)
{
    pid_t pid;
    void child_main(int, int,  int);

    if ((pid = fork()) < 0)
        err_sys("fork error");
    else if (pid > 0)
        return pid; /* parent */

    child_main(i, listenfd, addrlen); /* never returns */
}

void child_main(int i, int listenfd, int addrlen)
{
    int connfd;
    void web_child(int);
    socklen_t clilen;
    struct sockaddr *cliaddr;

    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    printf("child %ld starting\n", (long) getpid());

    for ( ; ; ) {
        clilen = addrlen;
        if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
            err_sys("accept error");

        web_child(connfd); /* process the request */
        if (close(connfd) < 0)
            err_sys("close error");    
    }
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