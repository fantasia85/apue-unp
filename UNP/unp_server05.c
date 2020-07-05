/* TCP预先派生子进程服务器程序，传递描述符
 * 只让父进程调用accept，然后把所接受的已连接套接字“传递”给某个子进程 */

#include "tcpfunc.h"
#include <errno.h>

typedef struct {
    pid_t child_pid; /* process ID */
    int child_pipefd; /* parent's stream pipe to/from child */
    int child_status; /* 0 = ready */
    long child_count; /* # connections handled */
} Child;

Child *cptr; /* aray of Child structures; calloc'ed */

static int nchildren;

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, i, navail, maxfd, nsel, connfd, rc;
    void sig_int(int);
    pid_t child_make(int, int, int);
    ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd);
    ssize_t n;
    fd_set rset, masterset;
    socklen_t addrlen, clilen;
    struct sockaddr *cliaddr;

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv05 [<host>] <port#> <#children>");

    FD_ZERO(&masterset);
    FD_SET(listenfd, &masterset);
    maxfd = listenfd;
    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    nchildren = atoi(argv[argc - 1]);
    navail = nchildren;
    if ((cptr = calloc(nchildren, sizeof(Child))) == NULL)
        err_sys("calloc error");

    /* preforking all the children */
    for (i = 0; i < nchildren; i++) {
        child_make(i, listenfd, addrlen); /* parent returns */
        FD_SET(cptr[i].child_pipefd, &masterset);
        maxfd = max(maxfd, cptr[i].child_pipefd);
    }

    signal(SIGINT, sig_int);

    for ( ; ; ) {
        rset = masterset;
        if (navail <= 0)
            FD_CLR(listenfd, &rset); /* turn off if no available children */
        if ((nsel = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0)
            err_sys("select error");
        
        /* check for new connections */
        if (FD_ISSET(listenfd, &rset)) {
            clilen = addrlen;
            if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
                err_sys("accept error");

            for (i = 0; i < nchildren; i++)
                if (cptr[i].child_status == 0)
                    break; /* available */
            
            if (i == nchildren)
                err_quit("no available children");
            cptr[i].child_status = 1; /* mark child as busy */
            cptr[i].child_count++;
            navail--;

            n = write_fd(cptr[i].child_pipefd, "", 1, connfd);
            if (close(connfd) < 0)
                err_sys("close error");

            if (--nsel == 0)
                continue; /* all done with select() results */
        }

        /* find any newly-available children */
        for (i = 0; i < nchildren; i++) {
            if (FD_ISSET(cptr[i].child_pipefd, &rset)) {
                if ((n = read(cptr[i].child_pipefd, &rc, 1)) == 0)
                    err_quit("child %d terminated unexpectedly", i);
                cptr[i].child_status = 0;
                navail++;
                if (--nsel == 0)
                    break; /* all done with select() results */
            }
        }
    }
}


void sig_int(int signo)
{
    int i;
    void pr_cpu_time(void);

    /* terminate all children */
    for (i = 0; i < nchildren; i++)
        kill(cptr[i].child_pid, SIGTERM);
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
    int sockfd[2];
    pid_t pid;
    void child_main(int, int, int);

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd) < 0)  /* 全双工 */
        err_sys("socketpair error");

    if ((pid = fork()) < 0)
        err_sys("fork error");
    else if (pid > 0) {
        if (close(sockfd[1]) < 0)
            err_sys("close error");
        cptr[i].child_pid = pid;
        cptr[i].child_pipefd = sockfd[0];
        cptr[i].child_status = 0;
        return pid; /* parent */
    }

    if (dup2(sockfd[1], STDERR_FILENO) < 0)
        err_sys("dup2 error"); /* child's stream pipe to parent */
    if (close(sockfd[0]) < 0)
        err_sys("close error");
    if (close(sockfd[1]) < 0)
        err_sys("close error");
    if (close(listenfd) < 0)
        err_sys("close error"); /* child does not need this open */
    child_main(i, listen, addrlen); /* never returns */
}

void child_main(int i, int listenfd, int addrlen)
{
    char c;
    int connfd;
    ssize_t n;
    void web_child(int);
    ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd);

    printf ("child %ld starting\n", (long) getpid());
    for ( ; ; ) {
        if ((n = read_fd(STDERR_FILENO, &c, 1, &connfd)) == 0)
            err_quit("read_fd returned 0");
        if (connfd < 0)
            err_quit("no descriptor from read_fd");

        web_child(connfd); /* process request */
        if (close(connfd) < 0)
            err_sys ("close error");

        if ((n = write(STDERR_FILENO, "", 1)) == 0)
            err_sys("write error"); /* tell parent we're ready again */
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


ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{

}

ssize_t write_fd (int fd, void *ptr, size_t nbytes, int sendfd)
{
    
}