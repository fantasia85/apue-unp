//tcp相关的自定义函数

#ifndef __TCPFUNC
#define __TCPFUNC

#include "apue.h"
#include <netdb.h>

struct addrinfo *host_serv(const char *hostname, const char *service, int family, int socktype);
int tcp_connect(const char *hostname, const char *service);
int tcp_listen(const char *hostname, const char *service, socklen_t *addrlenp);

#endif