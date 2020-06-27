#include "apue.h"
#include <errno.h> /* for definition of errno */
#include <stdarg.h> /* ISO C variable aruments */

static void err_doit(int, int, const char *, va_list);

/*
 * Nonfatal error related to a system call.
 * Print a message and return.
 */

void err_ret (const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */

void err_sys(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
    exit(1);
}

/* 
 * Nonfatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and return.
 */

void err_cont(int error, const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    err_doit(1, error, fmt, ap);
    va_end(ap);
}

/* 
 * Fatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and terminate.
 */
void err_exit (int error, const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    err_doit(1, error, fmt, ap);
    va_end(ap);
    exit(1);
}

/*
 * Fatal  error related to a system call.
 * Print a message, dump core, and terminate.
 */
void err_dump(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
    abort(); /* dump core and terminate */
    exit(1); /* shouldn't ger here */
}

/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return.
 */
void err_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(0, 0, fmt, ap);
    va_end(ap);
}

/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void err_quit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(0, 0, fmt, ap);
    va_end(ap);
    exit(1);
}

/* 
 * Print a message and return to caller.
 * Caller specifiess "errnoflag".
 */
static void err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
    char buf[MAXLINE];

    vsnprintf(buf, MAXLINE-1, fmt, ap);
    if (errnoflag)
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s", strerror(error));
        strcat (buf, "\n");
        fflush(stdout); /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(NULL); /* flushes all stdio output streams */
}




/*
 * Error routines for programs that can run as a daemon.
 */

#include <syslog.h> 

static void log_doit(int, int, int, const char *, va_list ap);

/*
 * Caller must define and set this: nonzero if
 * interactive, zero if daemon
 */
extern int log_to_stderr;

/* 
 * Initialize syslog(), if running as daemon.
 */
void log_open (const char *ident, int option, int facility)
{
    if (log_to_stderr == 0)
        openlog(ident, option, facility);
}

/*
 * Nonfatal error related to a system call.
 * Print a message with the system's errno value and return.
 */
void log_ret (const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_doit(1, errno, LOG_ERR, fmt, ap);
    va_end(ap);
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void log_sys(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_doit(1, errno, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(2);
}

/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return .
 */
void log_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_doit(0, 0, LOG_ERR, fmt, ap);
    va_end(ap);
}

/* 
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void log_quit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_doit(0, 0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(2);
}

/*
 * Fatal error related to a system call.
 * Error number passed as an explicit parameter.
 * Print a message and terminate.
 */
void log_exit(int error, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_doit(1, error, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(2);
}

/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag" and "Priority".
 */
static void log_doit(int errnoflag, int error, int priority, const char *fmt, va_list ap)
{
    char buf[MAXLINE];

    vsnprintf(buf, MAXLINE - 1, fmt, ap);
    if (errnoflag)
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s", strerror(error));
    strcat(buf, "\n");
    if (log_to_stderr)
    {
        fflush(stdout);
        fputs(buf, stderr);
        fflush(stderr);
    }
    else
    {
        syslog(priority, "%s", buf);
    } 
}


#include <fcntl.h>

void set_fl(int fd, int flags) /* flags are file status flags to turn on */
{
    int val;

    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("fcntl F_GETFL error");

    val |= flags; /* turn on flags */

    if (fcntl(fd, F_SETFL, val) < 0)
        err_sys("fcntl F_SETFL error");
}

void clr_fl(int fd, int flags) /* flags are file status flags to turn off */
{
    int val;

    if ((val = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("fcntl F_GETFL error");

    val &= ~flags; /* turn flags off */

    if (fcntl(fd, F_SETFL, val) < 0)
        err_sys("fcntl F_SETFL error");
}

ssize_t readn(int fd, void *vptr, size_t n) /* read n bytes from a descriptor. */
{
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0; /* and call read() again */
            else
            {
                return -1;
            }
        } 
        else if (nread == 0)
            break; /* EOF */

        nleft -= nread; 
        ptr += nread;
    }
    return n - nleft;
}

ssize_t writen(int fd, const void *vptr, size_t n) /* write n bytes to a descriptor */
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
            {
                return -1; /* error */
            }
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;  /* 写时一般不会发生写不足的情况，所以返回n */
}

/*
//readline() from UNP p74
static int read_cnt;
static char *read_ptr;
static char read_buf[MAXLINE];

// my_read 每次最多读取MAXLINE个字符，然后每次返回一个字符 
static ssize_t my_read(int fd, char *ptr) 
{
    if (read_cnt <= 0) {
        again:
            if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) { 
                if (errno == EINTR)
                    goto again;
                return -1;
            }
            else if (read_cnt == 0)
                return 0;
            read_ptr = read_buf;
    }

    read_cnt--;
    *ptr = *read_ptr++;
    return 1;
}


ssize_t readline (int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n') 
                break;  // newline is stored, like fgets() 
        }
        else if (rc == 0) {
            *ptr = 0;
            return n -1; // EOF, n - 1 bytes were read 
        }
        else
        {
            return -1; // error, errno set by read() 
        }
        
        *ptr = '\0'; // null terminate like fgets() 
        return n;
    }
}

ssize_t readlinebuf(void **vptrptr)
{
    if (read_cnt)
        *vptrptr = read_ptr;
    return read_cnt;
}
*/

/* PAINFULLY SLOW VERSION -- example only */
/*
ssize_t readline (int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char c, *ptr;

    ptr = vptr;
    for (n = 1; n < maxlen; n++) {
        again:
        if ( (rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break; // newline is stored, like fgets() 
        }
        else if (rc == 0) {
            *ptr = 0;
            return n - 1; // EOF, n - 1 bytes were read 
        }
        else
        {
            if (errno == EINTR)
                goto again;
            return -1; // error, errno set by read() 
        }
    }
    *ptr = 0; // null terminate like fgets() 
    return n;
}
*/