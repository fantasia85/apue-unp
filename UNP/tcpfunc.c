#include "tcpfunc.h"

/* 查找并转换主机名和服务名。成功时返回指向addrinfo结构的指针，若出错则为NULL */
struct addrinfo * host_serv(const char *host, const char *serv, int family, int socktype)
{
    int n;
    struct addrinfo hints, *res;

    bzero(&hints, sizeof (struct addrinfo));
    hints.ai_flags = AI_CANONNAME; /* always return canonical name */
    hints.ai_family = family; /* AD_UNSPEC, AF_INET, AF_INET6, etc. */
    hints.ai_socktype = socktype; /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        return NULL;

    return res; /* return pointer to first on linked list */
}

/* 创建一个TCP套接字并连接到一个服务器 */
int tcp_connect(const  char *host, const char *serv)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp_connect error for %s, %s: %s", host, serv, gai_strerror(n));
    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0)
            continue; /* ignore this one */

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
            break; /* success */
        
        if (close(sockfd) < 0)
            err_sys("close error");  /* ignore this one */
    }while ( (res = res->ai_next) != NULL);

    if (res == NULL)
        err_sys("tcp_connect error for %s, %s", host, serv);

    freeaddrinfo(ressave);
    return sockfd;
}

/* tcp_listen执行TCP服务器的通常步骤：创建一个TCP套接字，给它捆绑服务器的众所周知端口，并允许接受外来的
 * 连接请求。 */
int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
    int listenfd, n;
    const int on = 1;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp_listen error for %s, %s: %s", host, serv, gai_strerror(n));
    
    ressave = res;

    do {
        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listenfd < 0)
            continue; /* error, try next one */

        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
            err_sys("setsockopt error");

        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
            break; /* success */
        
        if (close(listenfd) < 0)
            err_sys("close error"); /* bind error, close and try next one */
    } while ((res = res->ai_next) != NULL);

    if (res == NULL)
        err_sys("tcp_listen error for %s, %s", host, serv); /* errno from final socket() or bind() */
    
    if (listen(listenfd, LISTENQ) < 0)
        err_sys("listen error");

    if (addrlenp)
        *addrlenp = res->ai_addrlen; /* return size of protocol address */

    freeaddrinfo(ressave);

    return listenfd;
}