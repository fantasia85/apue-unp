#include "apue.h"

void str_cli(FILE *fp, int sockfd)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];

    while (fgets(sendline, MAXLINE, fp) != NULL) {
        writen(sockfd, sendline, strlen(sendline));
        if (n = read(sockfd, recvline, MAXLINE) < 0)
            err_quit("str_cli: server terminated prematurely");
        recvline[n] = '\0';
        fputs(recvline, stdout);
    }
}