//非阻塞I/O模型，nonblock I/O

#include "apue.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

void str_cli(FILE *fp, int sockfd)
{
    int maxfdp1, val, stdineof;
    ssize_t n, nwritten;
    fd_set rset, wset;
    char to[MAXLINE], fr[MAXLINE];
    char *toiptr, *tooptr, *friptr, *froptr;

    /* 通过fcntl获取文件描述符的当前标志，并设置为非阻塞I/O */
    if ((val = fcntl(sockfd, F_GETFL, 0)) < 0)
        err_sys("fcntl get error");
    if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) < 0)
        err_sys("fcntl set error");

    if ((val = fcntl(STDIN_FILENO, F_GETFL, 0)) < 0)
        err_sys("fcntl get error");
    if (fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK) < 0)
        err_sys("fcntl set error");

    if ((val = fcntl(STDOUT_FILENO, F_GETFL, 0)) < 0)
        err_sys("fcntl get error");
    if (fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK) < 0)
        err_sys("fcntl set error");

    toiptr = tooptr = to; /* initialize buffer pointers */
    friptr = froptr = fr; 
    stdineof = 0;

    maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;
    for ( ; ; ) {
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        if (stdineof == 0 && toiptr < &to[MAXLINE])
            FD_SET(STDIN_FILENO, &rset); /* read from stdin */
        if (friptr < &fr[MAXLINE])
            FD_SET(sockfd, &rset);  /* read from socket */
        if (tooptr != toiptr)
            FD_SET(sockfd, &wset); /* data to write to socket */
        if (froptr != friptr)
            FD_SET(STDOUT_FILENO, &wset); /* data to write to stdout */

        if (select(maxfdp1, &rset, &wset, NULL, NULL) < 0)
            err_sys("select error");

        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if ( (n = read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("read error on stdin");
            }
            else if (n == 0) {
                fprintf (stderr, "%s: EOF on stdin\n", gf_time());
                stdineof = 1; /* all done with stdin */
                if (tooptr == toiptr)
                    if (shutdown(sockfd, SHUT_WR) < 0)
                        err_sys("shutdown error");  /* send FIN */
            }
            else {
                fprintf(stderr, "%s: read %d bytes from stdin\n", gf_time(), n);
                toiptr += n; /* just read */
                FD_SET(sockfd, &wset); /* try and write to socket below */
            }
        }

        if (FD_ISSET(sockfd, &rset)) {
            if ((n = read(sockfd, friptr, &fr[MAXLINE] - friptr)) < 0) {
                if (errno != EWOULDBLOCK)
                    err_sys("read error on socket");
            }
            else if (n == 0) {
                fprintf(stderr, "%s: EOF on socket\n", gf_time());
                if (stdineof)
                    return; /* normal termination */
                else
                    err_quit("str_cli: server terminated prematurely");
            }
            else {
                fprintf (stderr, "%s: read %d bytes from socket\n", gf_time(), n);
                friptr += n; /* just read */
                FD_SET(STDOUT_FILENO, &wset); /* try and write blow */
            }
            
            if (FD_ISSET(STDOUT_FILENO, &wset) && ( (n = friptr - froptr) > 0)) {
                if ( (nwritten = write(STDOUT_FILENO, froptr, n)) < 0) {
                    if (errno != EWOULDBLOCK)
                        err_sys("write error to stdout");
                }
                else {
                    fprintf (stderr, "%s: wrote %d bytes to stdout\n", gf_time(), nwritten);
                    froptr += nwritten; /* just written */
                    if (froptr == friptr)
                        froptr = friptr = fr; /* back to beginning of buffer */
                }
            }

            if (FD_ISSET(sockfd, &wset) && ((n = toiptr - tooptr) > 0)) {
                if ( (nwritten = write(sockfd, tooptr, n)) < 0) {
                    if (errno != EWOULDBLOCK)
                        err_sys("write error to socket");
                }
                else {
                    fprintf(stderr, "%s: wrote %d bytes to socket\n", gf_time(), nwritten);
                    tooptr += nwritten; /* just written */
                    if (tooptr == toiptr) {
                        toiptr = tooptr = to; /* back to beginning of buffer */
                        if (stdineof)
                            if (shutdown(sockfd, SHUT_WR) < 0)
                                err_sys("shutdown error"); /* send FIN */
                    }
                }
            }
        }
    }  
}