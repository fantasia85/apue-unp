//使用poll来处理任意个客户的单进程程序，见UNP P146

#include "apue.h"
#include <sys/select.h>
#include <netinet/in.h>
#include <poll.h>
#include <limits.h> /* for OPEN_MAX */
#include <errno.h>

#define INFTIM -1 /* 用于poll函数中，定义timeout参数为永远等待 */

int log_to_stderr = 0;

extern void str_echo(int sockfd);

int main(int argc, char **argv)
{
    int i, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    socklen_t clilen;
    long openmax = sysconf(_SC_OPEN_MAX);
    /* 获得每个进程最大打开文件数，long sysconf(int name);函数见APUE P34 */
    struct pollfd client[openmax];
    struct sockaddr_in cliaddr, servaddr;

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        log_sys("bind error");

    if (listen(listenfd, LISTENQ) < 0)
        log_sys("listen error");

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM; //普通数据可读
    for (i = 1; i < openmax; i++)
        client[i].fd = -1; /* -1 indicates available entry */
    maxi = 0; /* max index into client[] array */
    
    for ( ; ; ) {
        if ((nready = poll(client, maxi + 1, INFTIM)) < 0)
            log_sys("poll error");

        if (client[0].revents & POLLRDNORM) { /* new client connection */
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
                log_sys("accept error");

            for (i = 1; i < openmax; i++)
                if (client[i].fd < 0) {
                    client[i].fd = connfd; /* save descriptor */
                    break;
                }
            
            if (i == openmax) 
                log_quit("too many clients");

            client[i].events = POLLRDNORM;
            if (i > maxi)
                maxi = i; /* max index in client[] array */

            if (--nready <= 0)
                continue; /* no more readable  descriptor */
        }

        for (i = 1; i <= maxi; i++) { /* check all slients for data */
            if ((sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) {
                if ( (n = read(sockfd, buf, MAXLINE)) < 0) {
                    if (errno == ECONNRESET) {
                        /* connection reset by client */
                        if (close(sockfd) < 0)
                            log_sys("close error");
                            client[i].fd = -1;
                    }
                    else
                        log_sys("read error");                    
                }
                else if (n == 0) {
                    /* connection closed by client */
                    if (close(sockfd) < 0)
                        log_sys("close error");
                    client[i].fd = -1;
                }
                else {
                    if ((n = writen(sockfd, buf, n)) < 0)
                        log_sys("writen error");
                }

                if (--nready <= 0)
                    break; /* no more readable descriptors */
            }
        }
    }
}