//非阻塞connect：web客户程序

#include "apue.h"
#include <netdb.h>

#define MAXFILES 20
#define SERV "80"  /* port number or service name */

struct file {
    char *f_name; /* filename */
    char *f_host; /* hostname or IPv4/IPv6 address */
    int f_fd; /* descriptor */
    int f_flags; /* F_xxx below */
}file[MAXFILES];

#define F_CONNECTING 1 /* connect() in process */
#define F_READING  2 /* connect() complete; now reading  */
#define F_NONE 4 /* all done */

#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"

/* global */
int nconn, nfiles, nlefttoconn, nlefttoread, maxfd;
fd_set rset, wset;

/* function prototypes */
struct addrinfo *host_serv(const char *hostname, const char *service, int family, int socktype);
int tcp_connect(const char *host, const char *serv);
void home_page (const char *, const char *);
void start_connect(struct file *);
void write_get_cmd(struct file *);
