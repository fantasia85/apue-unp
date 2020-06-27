#include "apue.h"
#include <errno.h>

void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];

again:
    while ( (n = read(sockfd, buf, MAXLINE)) > 0) /* 从套接字读入数据 */
        writen(sockfd, buf, n); /* 把内容回射给客户 */

    if (n < 0 && errno == EINTR)  /*当出错或者被中断后，继续读取 */
        goto again;
    else if (n < 0)
        err_sys("str_echo: read error");
}