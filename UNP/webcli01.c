/* 该版本在本系统（CentOS）下实际无法运行，因为无法包含Solaris threads的<thread.h>头文件 */

//Web客户程序，把它重新编写成用线程代替非阻塞connect

/*
#include "tcpfunc.h"
#include <thread.h> // Solaris threads

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

#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"

int nconn, nfiles, nlefttoconn, nlefttoread;

void *do_get_read(void *);
void home_page(const char *, const char *);
void write_get_cmd(struct file *);

int main(int argc, char **argv)
{
    int i, n, maxnnconn;
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
            // find a file to read
            for (i = 0; i < nfiles; i++) 
                if (file[i].f_flags == 0)
                    break;
            if (i == nfiles)
                err_quit("nlefttoconn = %d but nothing foud", nlefttoconn);

            file[i].f_flags = F_CONNECTING；
            if (pthread_create(&tid, NULL, &do_get_read, &file[i]) != 0)
                err_sys("pthread_create error");
            file[i].f_tid = tid;
            nconn++;
            nlefttoconn--;
        }

        // Solaris线程函数thr_join，等待任何一个线程终止，Pthreads没有提供等待任一线程终止的手段
        if ((n = thr_join(0, &tid, (void **) &fptr)) != 0)
            errno = n, err_sys("thr_join error");

        nconn--;
        nlefttoread--;
        printf("thread id %d for %s done\n", tid, fptr->f_name);
    }

    exit(0);
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
    fptr->f_flags = F_DONE; // clears F_READING

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
*/