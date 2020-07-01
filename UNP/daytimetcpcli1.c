//非阻塞connect：时间获取客户程序
//由daytimetcpcli.c

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SA struct sockaddr 

int log_to_stderr = 1; /* 见apue.c中的定义，非守护进程定义为非零值 */

extern int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec);

int main(int argc, char **argv)
{
    int sockfd, n;
    char recvline[MAXLINE + 1];
    struct sockaddr_in servaddr;

    if (argc != 2)
        err_quit("usage: a.out <IPaddress>");

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13); /* daytime server */
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) /* 地址转换函数，将ASCII字符串转换为网络子节序的二进制值 */
        err_quit("inet_pton error for %s", argv[1]);

    if (connect_nonb(sockfd, (SA *) &servaddr, sizeof(servaddr), 0) < 0)
        err_sys("connect error");

    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = '\0'; /* 如果为0则会导致fputs接受不到EOF的标志，此处应该改为'\0'才能正确表示字符串的结束*/
        if (fputs(recvline, stdout) == EOF)
        //if (write(STDOUT_FILENO, recvline, n) > 0)
            err_sys("fput error");
    }

    if (n < 0)
        err_sys("read error");

    exit(0);
}