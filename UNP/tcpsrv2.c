#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/wait.h>

extern void str_echo(int sockfd);

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    void sig_chld(int); /* 调用waitpid函数的信号处理函数 */
    
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if ( bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        log_sys("bind error");

    if ( listen(listenfd, LISTENQ) < 0)
        log_sys("listen error");

    signal(SIGCHLD, sig_chld); /* 配置信号处理函数 */ 

    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR)
                continue;  /* back to for() */
            else
                log_sys("accept error");            
        }
        if ( (childpid = fork()) < 0)
            log_sys("fork error");

        if (childpid == 0) { /* child process */
            if (close(listenfd) < 0) /* close listening socket */
                log_sys("child close listenfd error");
            str_echo(connfd); /* process the request */
            if (close(connfd) < 0)
                log_sys("child close connfd error");
            exit (0); /* 退出时自动关闭打开的文件描述符 */
        }
        if (close(connfd) < 0) /* parent closes connected socket */
            log_sys("parent close connfd error");
    }    
}


void sig_chld(int signo)
{
    pid_t pid;
    int stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) /* -1表示等待第一个终止的子进程。 指定WNOHANG选项，告知waitpid在有尚未终止的
                                                        子进程在运行时不要阻塞 */
        printf("child %d terminated\n", pid);

    return;
}
