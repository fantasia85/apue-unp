#include "apue.h"
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  /* for inet_ntop() */

#define SA struct sockaddr

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, connfd, n;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char buff[MAXLINE];
    time_t ticks;

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0))  < 0)
        log_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13); /* daytime server */

    if (bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        log_sys("bind error");
    
    if (listen(listenfd, LISTENQ) < 0)
        log_sys("listen error");

    for ( ; ; ) {
        len = sizeof(cliaddr);
        if (connfd = accept(listenfd, (SA *) NULL, NULL) < 0)
            log_sys("accept error");

        printf ("connection from %s, port %d\n", inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)), ntohs(cliaddr.sin_port));
        //显示客户端的IP地址和端口号

        ticks = time (NULL);
        snprintf(buff, sizeof (buff), "%.24s\r\n", ctime(&ticks));
        if ( (n = write(connfd, buff, strlen(buff))) < 0)
            log_sys("write error");

        if (close(connfd) < 0)
            log_sys("close error");
    }
}