//使用线程的TCP回射服务器程序

#include "tcpfunc.h"
#include <pthread.h>

int log_to_stderr = 0;

extern void str_echo (int sockfd);

static void *doit(void *arg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr *cliaddr;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: tcpserv01 [ <host> ] | <services or port>");

    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("cliaddr error");

    for ( ; ; ) {
        len = addrlen;
        if ((connfd = accept(listenfd, cliaddr, &len)) < 0)
            err_sys("connfd error");
        if (pthread_create(&tid, NULL, &doit, (void *) connfd) != 0)
            err_sys("pthread_create error");
    }
}

static void *doit (void *arg)
{
    if (pthread_detach(pthread_self()) != 0)
        err_sys("pthread_detach error");
    str_echo((int) arg); /* same function as before */
    if (close((int) arg) < 0) /* done with connected socket */
        err_sys("close error");
    return NULL;
}