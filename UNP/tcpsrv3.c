//使用select来处理任意个客户的单进程程序，见UNP P138

#include "apue.h"
#include <sys/select.h>
#include <netinet/in.h>

int log_to_stderr = 0;

extern void str_echo(int sockfd);

int main(int argc, char **argv)
{
    int i, maxi, maxfd, listenfd, connfd, sockfd; /* maxi指示client数组当前使用项的最大下标 */
    int nready, client[FD_SETSIZE]; 
    ssize_t n;
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        err_sys("bind error");

    if (listen(listenfd, LISTENQ) < 0)
        err_sys("listen error");

    maxfd = listenfd; /* initialize */
    maxi = -1; /* index into client[] array */
    for (i = 0; i < FD_SETSIZE; i++) 
        client[i] = -1; /* -1 indicates available entry */

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for ( ; ; ) {
        rset = allset;  /* structure assignment */
        if ((nready = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0)
            err_sys("select error");
        
        if (FD_ISSET(listenfd, &rset)) { /* new client connection */
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
                err_sys("accept error");

            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {  // 找到第一个可用项（值为-1），在当前位置保存新的已连接描述符
                    client[i] = connfd; /* save descriptor */
                    break;
                }
            }

            if (i == FD_SETSIZE)
                err_quit("too many clients");

            FD_SET(connfd, &allset); /* add new descriptor to set */
            if (connfd > maxfd)
                maxfd = connfd; /* for select */
            if (i > maxi)
                maxi = i; /* max index in client[] array */
            if (--nready <= 0) 
                continue; /* no more readable descriptors */
        }

        for (i = 0; i <= maxi; i++) { /* check all clients for data */
            if ( (sockfd = client[i]) < 0)
                continue;

            if (FD_ISSET(sockfd, &rset)) {
                if ( (n = read(sockfd, buf, MAXLINE)) < 0)
                    err_sys("read error");
                else if (n == 0) {
                    /* connection closed by client */
                    if (close(sockfd) < 0)
                        err_sys("close error");
                    FD_CLR(sockfd, &allset);
                    client[i] = -1;
                }
                else
                {
                    if ( (n = writen(sockfd, buf, n)) < 0)
                        err_sys("writen error");
                }

                if (--nready <= 0)
                    break; /* no more readable descriptors */
            }
        }
    }
}