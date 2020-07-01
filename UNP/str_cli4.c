//使用两个进程的str_cli函数
//子进程把来自服务器的文本行复制到标准输出，父进程把来自标准输入的文本行复制到服务器

#include "apue.h"
#include <sys/socket.h>

void str_cli(FILE *fp, int sockfd)
{
    pid_t pid;
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;

    if ( (pid = fork()) < 0)
        err_sys("fork error");
    else if (pid == 0) {  /* child : server -> stdout */
        while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
            if (fputs(recvline, stdout) == EOF)
                err_sys("fputs error");
        }

        kill (getppid(), SIGTERM); /* in case parent still running */
        exit (0);
    }

    /* parent: stdin -> server */
    while (fgets(sendline, MAXLINE, fp) != NULL) {
        if ( (n = writen(sockfd, sendline, strlen(sendline) + 1)) < 0)  
        /* writen的第三个参数应该改为strlen(sendline) + 1，原因在于strlen()函数每次获得的长度不包含'\0'，
            因此调用writen往缓存区存入的最后一个字符为'\n'。同时，下一次写的长度若小于第一次的长度，则
            在写入'\n'之后，会继续写入缓存区中原来存在的内容。因此，长度应该+1，把'\0'一同写入缓存区 */
            err_sys("writen error");
    }

    if (shutdown(sockfd, SHUT_WR) < 0) /* EOF on stdin, send FIN */
        err_sys("shutdown error");
    pause();
    return;
}