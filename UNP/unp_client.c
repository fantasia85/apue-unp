//TCP测试用客户程序
/* 执行本客户程序的命令为: $ ./unp_client 127.0.0.1 8888 5 500 4000 
 * 表示将建立2500个与服务器的TCP连接，5个子进程各自发起500次连接。在每个连接上向服务器发送5字节
 * 数据("4000\n")，服务器向客户返送4000字节数据。 */

#include "tcpfunc.h"
#include <errno.h>
#include <sys/wait.h>

#define MAXN 16384 /* max # bytes to request from server */

int log_to_stderr = 1;

int main(int argc, char **argv)
{
    int i, j, fd, nchildren, nloops, nbytes;
    pid_t pid;
    ssize_t n;
    char request[MAXLINE], reply[MAXN];

    if (argc != 6)
        err_quit("usage: client <hostname or IPaddr> <port> <#children> <#loops/child> <#bytes/request>");

    nchildren = atoi(argv[3]);
    nloops = atoi(argv[4]);
    nbytes = atoi(argv[5]);
    snprintf(request, sizeof(request), "%d\n", nbytes); /* newline at end */

    for (i = 0; i < nchildren; i++) {
        if ((pid = fork()) < 0)
            err_sys("fork error");
        else if (pid == 0) { /* child */
            for (j = 0; j < nloops; j++) {
                fd = tcp_connect(argv[1], argv[2]);

                if ((n = write(fd, request, strlen(request))) < 0)
                    err_sys("write error");

                if ((n = readn(fd, reply, nbytes)) != nbytes)
                    err_quit("server returned %d bytes", n);

                if (close(fd) < 0)
                    err_sys("close error"); /* TIME_WAIT on client, not server */
            }
            printf("child %d done\n", i);
            exit(0);
        }
        /* parent loops around to fork() again */
    }

    while (wait(NULL) > 0) /* now parent waits for all children */
        ;
    if (errno != ECHILD)
        err_sys("wait error");

    exit(0);
}