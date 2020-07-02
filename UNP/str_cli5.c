//使用线程的str_cli函数

#include "apue.h"
#include <pthread.h>
#include <sys/socket.h>

void *copyto (void *);

static int sockfd; /* global for both threads to access */
static FILE *fp;

void str_cli(FILE *fp_arg, int sockfd_arg)
{
    char recvline[MAXLINE];
    pthread_t tid;
    int n;

    sockfd = sockfd_arg; /* copy arguments to externals */
    fp = fp_arg;

    if (pthread_create(&tid, NULL, copyto, NULL) != 0)
        err_sys("pthread_create error");

    while ((n = read(sockfd, recvline, MAXLINE)) > 0)
        if (fputs(recvline, stdout) < 0)
            err_sys("fputs error");
}

void *copyto(void *arg)
{
    char sendline[MAXLINE];
    int n;

    while (fgets(sendline, MAXLINE, fp) != NULL)
        if ((n = writen(sockfd, sendline, strlen(sendline) + 1)) < 0)
            err_sys("writen error");

    if (shutdown(sockfd, SHUT_WR) < 0)
        err_sys("shutdown error"); /* EOF on stdin, send FIN */

    return NULL; /* return (i.e., thread terminates) when EOF on stdin */
}