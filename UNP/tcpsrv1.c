//并发服务器模型
//服务器端

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>

extern void str_echo(int sockfd);

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_sys("socket error");
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
        log_sys("bind error");

    if (listen(listenfd, LISTENQ) < 0)
        log_sys("listen error");
    
    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        if (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen) < 0)
            log_sys("accept error");
        if ( (childpid = fork()) < 0)
            log_sys("fork error");
        
        if (childpid == 0) { /* child process */
            if (close(listenfd) < 0) /* close listening socket */
                log_sys("child listenfd close error");
            str_echo(connfd); /* process the request */
            if (close(connfd) < 0)
                log_sys("child connfd close error"); /* done with this client*/
                //实际可以省略这一步，因为在exit时会自动关闭已打开的文件描述符
            exit (0);
        }

        if (close(connfd) < 0)
            log_sys("parent connfd close error");  /* parent closes connected socket */
    }
}