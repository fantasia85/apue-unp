//本版本使用I/O复用的selete函数

#include "apue.h"
#include <sys/select.h>

void str_cli(FILE *fp, int sockfd)
{
    int maxfdp1, n;
    fd_set rset;
    char sendline[MAXLINE], recvline[MAXLINE];

    FD_ZERO(&rset);
    for ( ; ; ) {
        //每次都需要重新设置
        FD_SET(fileno(fp), &rset); //fileno()函数将标准I/O文件指针转换为对应的描述符
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1; //描述符个数为最大文件描述符+1

        if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
            err_sys("select error");

        if (FD_ISSET(sockfd, &rset)) {  /* socket is readable */
            if ((n = read(sockfd, recvline, MAXLINE)) < 0)
                err_quit("str_cli: server terminated prematurely");
            recvline[n] = '\0';
            fputs(recvline, stdout);
        }

        if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
            if (fgets(sendline, MAXLINE, fp) == NULL)
                return; /* all done */
            if ((n = writen(sockfd, sendline, strlen(sendline))) < 0)
                err_sys("writen error");
        }
    }
}