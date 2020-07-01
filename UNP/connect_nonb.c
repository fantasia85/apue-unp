#include "apue.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

int connect_nonb(int sockfd, const struct sockaddr *saptr, socklen_t salen, int nsec)
{
    int flags, n, error;
    socklen_t len;
    fd_set rset, wset;
    struct timeval tval;

    if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0)
        err_sys("fcntl error");
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        err_sys("fcntl error");

    error = 0;
    if ((n = connect(sockfd, saptr, salen)) < 0)
        if (errno != EINPROGRESS)
            return -1;

    /* Do whatever we want while the connect is taking place */

    if (n == 0)
        goto done; /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;
    if ( (n = select(sockfd + 1, &rset, &wset, NULL, nsec ? &tval : NULL)) < 0) 
        err_sys("select error");
    else if (n == 0) {
        close(sockfd); /* timeout */
        errno = ETIMEDOUT;
        return -1;
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return -1; /* Solaris pending error */
    }
    else 
        err_quit("select error: sockfd not set");

done:
    if (fcntl(sockfd, F_SETFL, flags)); /* restore file status flags */
    if (error) {
        if (close(sockfd))
            err_sys ("close error"); /* just in case */
        errno = error;
        return -1;
    }
    return 0;
}