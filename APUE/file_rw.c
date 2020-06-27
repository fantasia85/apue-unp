#include "apue.h"

#define BUFFSIZE 4096

int log_to_stderr = 1; //将log_to_stderr设置为非0值，在这种情况下，出错消息被发送至标准错误

int main()
{
    int n;
    char buf[BUFFSIZE];

    while ((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0) /* 从标准输入读取字符到buf中 */
        if (write(STDIN_FILENO, buf, n) != n) /* 将buf的数据写至标准输出 */
            err_sys("write error");

    if (n < 0)
        err_sys("read error");

    exit(0);
}
