//非阻塞connect：web客户程序

#include "apue.h"
#include "webcli.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

int log_to_stderr = 1;

int main(int argc, char **argv)
{
    int i, fd, n, maxnconn, flags, error;
    char buf[MAXLINE];
    fd_set rs, ws;
    if (argc < 5)
        err_quit("usage: web <#conns> <hostname> <homepage> <file1> ...");

    maxnconn = atoi(argv[1]);

    nfiles = min (argc - 4, MAXFILES);
    for (i = 0; i < nfiles; i++) {
        file[i].f_name = argv[i + 4];
        file[i].f_host = argv[2];
        file[i].f_flags = 0;
    }
    printf("nfiles = %d\n", nfiles);

    home_page(argv[2], argv[3]);

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    maxfd = -1;
    nlefttoread = nlefttoconn = nfiles;
    nconn = 0;

    while (nlefttoread > 0) {
        while (nconn < maxnconn && nlefttoconn > 0) {
            /* find a file to read */
            for (i = 0; i < nfiles; i++)
                if (file[i].f_flags == 0)
                    break;
            if (i == nfiles)
                err_quit("nlefttoconn = %d but nothing found", nlefttoconn);
            start_connect(&file[i]);
            nconn++;
            nlefttoconn--;
        }

        rs = rset;
        ws = wset;
        if ((n = select(maxfd + 1, &rs, &ws, NULL, NULL)) < 0)
            err_sys("select error");

        for (i = 0; i < nfiles; i++) {
            flags = file[i].f_flags;
            if (flags == 0 || flags & F_DONE)
                continue;
            fd = file[i].f_fd;
            if (flags & F_CONNECTING && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))) {
                n = sizeof (error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0) {
                    err_ret("nonblocking connect failed for %s", file[i].f_name);
                }

                /* connection established */
                printf("connection established for %s\n", file[i].f_name);
                FD_CLR(fd, &wset); /* no more writeablility test */
                write_get_cmd(&file[i]); /* write() the GET command */
            }
            else if (flags & F_READING &&  FD_ISSET(fd, &rs)) {
                if ((n = read(fd, buf, sizeof(buf))) < 0)
                    err_sys("read error");
                else if (n == 0) {
                    printf("end-of-file on %s\n", file[i].f_name);
                    if (close(fd) < 0)
                        err_sys("close error");
                    file[i].f_flags = F_DONE; /* clears F_READING */
                    FD_CLR(fd, &rset);
                    nconn--;
                    nlefttoread--;
                }
                else
                    printf("read %d bytes from %s\n", n, file[i].f_name);
            }
        }
    }
    exit (0);
}

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

/* home_page 函数创建一个TCP连接，发出一个命令到服务器，然后读取主页。 */
void home_page (const char *host, const char *fname)
{
    int fd, n, nwritten;
    char line[MAXLINE];

    fd = tcp_connect(host, SERV); /* blocking connect() */

    n = snprintf(line, sizeof(line), GET_CMD, fname);
    if ((nwritten = writen(fd, line, n)) < 0)
        err_sys("writen error");
    for ( ; ; ) {
        if ((n = read(fd, line, MAXLINE)) < 0)
            err_sys("read error");
        else if (n == 0)
            break;

        printf("read %d bytes of home page\n", n);
        /* do whatever with data */
    }

    printf ("end-of-file on home page\n");
    if (close(fd) < 0)
        err_sys("close error");
}

/* 发起非阻塞connect */
void start_connect (struct file *fptr)
{
    int fd, flags, n;
    struct addrinfo *ai;

    if ((ai = host_serv(fptr->f_host, SERV, 0, SOCK_STREAM)) == NULL)
        err_sys("host_serv error");

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
        err_sys("socket error");
    fptr->f_fd = fd;
    printf("start_connect for %s, fd %d\n", fptr->f_name, fd);

    /* set socket nonblocking */
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("fcntl get error");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        err_sys("fcntl set error");

    /* Initiate nonblocking connect to the server. */
    if ((n = connect(fd, ai->ai_addr, ai->ai_addrlen)) < 0) {
        if (errno != EINPROGRESS)
            err_sys("nonblocking  connect error");
        fptr->f_flags = F_CONNECTING;
        FD_SET(fd, &rset); /* select for reading and writing  */
        FD_SET(fd, &wset);
        if (fd > maxfd)
            maxfd = fd;
    }
    else if (n >= 0) {  /* connect is already done */
        write_get_cmd(fptr);  /* write() the GET command */
    }
}

/* 发送一个HTTP GET命令到服务器 */
void write_get_cmd(struct file *fptr)
{
    int n, nwritten;
    char line[MAXLINE];

    n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
    if ((nwritten = writen(fptr->f_fd, line, n)) < 0)
        err_sys("writen error");
    printf("wrote %d bytes for %s\n", n, fptr->f_name);

    fptr->f_flags = F_READING; /* clears F_CONNECTING */

    FD_SET(fptr->f_fd, &rset); /* will read server's reply */
    if (fptr->f_fd > maxfd)
        maxfd = fptr->f_fd;
}