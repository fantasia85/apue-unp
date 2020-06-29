//使用select和shutdown函数，shutdown函数用于处理批量输入

#include "apue.h"
#include <sys/select.h>
#include <sys/socket.h>

void str_cli(FILE *fp, int sockfd)
{
    int maxfdp1, stdineof;
    fd_set rset;
    char buf[MAXLINE];
    int n;

    stdineof = 0; //用以标志在标准输入上是否遇到EOF
    FD_ZERO(&rset);
    for ( ; ; ) {
        if (stdineof == 0)  //当遇到EOF后，select中就不再设置
            FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1; //描述符的个数为最大的文件描述符+1
        if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
            err_sys("select error");
        
        if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
            if ( (n = read(sockfd, buf, MAXLINE)) < 0)
                err_sys("read error");
            else if (n == 0) {
                if (stdineof == 1)
                    return;  /* normal termination */
                else
                    err_quit("str_cli: server terminated prematurely");
            }

            if ( (n = write(fileno(stdout), buf, n)) < 0)
                err_sys("write error");
        }

        if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
            if ( (n = read(fileno(fp), buf, MAXLINE)) < 0)
                err_sys("read error");
            else if (n == 0) {
                stdineof = 1;
                if (shutdown(sockfd, SHUT_WR) < 0) /* send FIN */
                                                    //关闭套接字的写端
                    err_sys("shutdown error");
                FD_CLR(fileno(fp), &rset);
                continue;               
            }

            if ( (n = writen(sockfd, buf, n)) < 0)
                err_sys("writen error");
        }
    }
}