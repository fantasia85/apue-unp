//Web客户程序，把它重新编写成用线程代替非阻塞connect
//使用互斥锁和条件变量，把Solaris threads的thr_join函数的调用替换成调用pthread_join.

#include "tcpfunc.h"
#include <errno.h>

#define MAXFILES 20 
#define SERV "80" // port number or service name 

struct file {
    char *f_name; // filename
    char *f_host; // hostname or IP address 
    int f_fd; // descriptor
    int f_flags; // F_xxx below
    pthread_t f_tid; // thread ID
} file[MAXFILES];

#define F_CONNECTING  1 //connect() in process
#define F_READING 2 // connect() complete; now reading
#define F_DONE 4 // all done
#define F_JOINED 8 /* main has pthread_join'ed */

#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"

int nconn, nfiles, nlefttoconn, nlefttoread;

int ndone; /* number of terminated threads */
pthread_mutex_t ndone_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ndone_cond = PTHREAD_COND_INITIALIZER;

void *do_get_read(void *);
void home_page(const char *, const char *);
void write_get_cmd(struct file *);

int log_to_stderr = 1;

int main(int argc, char **argv)
{
    int i, n, maxnconn;
    pthread_t tid;
    struct file *fptr;

    if (argc < 5)
        err_quit("usage: web <#conns> <IPaddr> <homepage> file1 ...");
    maxnconn = atoi(argv[1]);

    nfiles = min(argc - 4, MAXFILES);
    for (i = 0; i < nfiles; i++) {
        file[i].f_name = argv[i + 4];
        file[i].f_host = argv[2];
        file[i].f_flags = 0;
    }
    printf("nfiles = %d\n", nfiles);

    home_page(argv[2], argv[3]);

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

            file[i].f_flags = F_CONNECTING;
            if (pthread_create(&tid, NULL, &do_get_read, &file[i]) != 0)
                err_sys("pthread_create error");
            file[i].f_tid = tid;
            nconn++;
            nlefttoconn--;
        }

        /* wait for thread to terminate */
        if (pthread_mutex_lock(&ndone_mutex) != 0)
            err_sys("pthread_mutex_lock  error");
        while (ndone == 0)
            if (pthread_cond_wait(&ndone_cond, &ndone_mutex) != 0)
                err_sys("pthread_cond_wait error");
        
        for (i = 0; i < nfiles; i++) {
            if (file[i].f_flags & F_DONE) {
                if (pthread_join(file[i].f_tid, (void **) &fptr) != 0)
                    err_sys("pthread_join error");
                
                if (&file[i] != fptr)
                    err_quit("file[i] != fptr");
                fptr->f_flags = F_JOINED; /* clear F_DONE */
                ndone--;
                nconn--;
                nlefttoread--;
                printf("thread %d for %s done\n", fptr->f_tid, fptr->f_name);
            }
        }
        if (pthread_mutex_unlock(&ndone_mutex) != 0)
            err_sys("pthread_mutex_unlock error");
    }

    exit (0);
}

// home_page 函数创建一个TCP连接，发出一个命令到服务器，然后读取主页。 
void home_page (const char *host, const char *fname)
{
    int fd, n, nwritten;
    char line[MAXLINE];

    fd = tcp_connect(host, SERV); // blocking connect() 

    n = snprintf(line, sizeof(line), GET_CMD, fname);
    if ((nwritten = writen(fd, line, n)) < 0)
        err_sys("writen error");
    for ( ; ; ) {
        if ((n = read(fd, line, MAXLINE)) < 0)
            err_sys("read error");
        else if (n == 0)
            break;

        printf("read %d bytes of home page\n", n);
        // do whatever with data 
    }

    printf ("end-of-file on home page\n");
    if (close(fd) < 0)
        err_sys("close error");
}

void *do_get_read(void *vptr)
{
    int fd, n;
    char line[MAXLINE];
    struct file *fptr;

    fptr = (struct file *) vptr;

    fd = tcp_connect(fptr->f_host, SERV);
    fptr->f_fd = fd;
    printf("do_get_read for %s, fd %d, thread %d\n", fptr->f_name, fd, fptr->f_tid);

    write_get_cmd(fptr); // write() the GET command 

    //Read server's reply
    for ( ; ; ) {
        if ((n = read(fd, line, MAXLINE)) == 0)
            break; // server closed connection 
        printf ("read %d bytes from %s\n", n, fptr->f_name);
    }

    printf("end-of-file on %s\n", fptr->f_name);
    if (close(fd) < 0)
        err_sys("close error");

    if (pthread_mutex_lock(&ndone_mutex) != 0)
        err_sys("pthread_mutex_lock error");
    fptr->f_flags = F_DONE; // clears F_READING
    ndone++;
    if (pthread_cond_signal(&ndone_cond) != 0)
        err_sys("pthread_cond_signal error");
    if (pthread_mutex_unlock(&ndone_mutex) != 0)
        err_sys("pthread_mutex_unlock error");

    return fptr; // terminate thread
}

// 发送一个HTTP GET命令到服务器。不调用FD_SET，也不使用maxfd
void write_get_cmd(struct file *fptr)
{
    int n, nwritten;
    char line[MAXLINE];

    n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
    if ((nwritten = writen(fptr->f_fd, line, n)) < 0)
        err_sys("writen error");
    printf("wrote %d bytes for %s\n", n, fptr->f_name);

    fptr->f_flags = F_READING; // clears F_CONNECTING

    //FD_SET(fptr->f_fd, &rset); // will read server's reply
    //if (fptr->f_fd > maxfd)
    //    maxfd = fptr->f_fd;
}
