//使用线程且参数传递更具移植性的TCP回射服务器程序
//每次传递已连接描述符时，采用malloc重新分配一个整数变量的内存空间

#include "tcpfunc.h"
#include <pthread.h>

int log_to_stderr = 0;

extern void str_echo(int sockfd);

static void *doit(void *); /* each thread executes this function */

int main(int argc, char **argv)
{
    int listenfd, *iptr;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr *cliaddr;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: tcpsrv6 [<host>] <service or port>");

    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    for ( ; ; ) {
        len = addrlen;
        /* 每次用malloc为已连接描述符为分配空间，将指针传入线程的函数中 */
        if ((iptr = malloc(sizeof(int))) == NULL)
            err_sys("malloc error");
        if ((*iptr = accept(listenfd, cliaddr, &len)) < 0)
            err_sys("accept error");
        if (pthread_create(&tid, NULL, &doit, iptr) != 0)
            err_sys("pthread create error");
    }
}

static void *doit(void *arg)
{
    int connfd;

    connfd = *((int *) arg);
    free(arg);

    if (pthread_detach(pthread_self()) != 0)
        err_sys("pthread detach error");

    str_echo(connfd); /* same function as before */
    if (close(connfd) < 0)
        err_sys("close error"); /* done with connected socket */
    return NULL;
}