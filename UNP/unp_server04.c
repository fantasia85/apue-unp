/* TCP预先派生子进程服务器程序，accept使用线程上锁保护 */
/* 在不同进程之间使用线程上锁要求：1. 互斥锁变量必须存放在由所有进程共享的内存区中， 2. 必须告知线程函数库这是在不同进程之间
 * 共享的互斥锁 */

#include "tcpfunc.h"
#include <errno.h>
#include <sys/wait.h>

static int nchildren;  /* 预先派生的子进程数量 */
static pid_t *pids; /* 用以保留各子进程pid的数组 */

int log_to_stderr = 0;

int main(int argc, char **argv)
{
    int listenfd, i;
    socklen_t addrlen;
    void sig_int(int);
    pid_t child_make(int, int, int); /* 创建各个子进程 */
    
    void my_lock_init (char *pathname); 

    if (argc == 3)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 4)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: serv02 [<host>] <port#> <#children>");

    nchildren = atoi(argv[argc - 1]); //预先派生的子进程个数
    if ((pids = calloc(nchildren, sizeof(pid_t))) == NULL)
        err_sys("calloc error"); //为每个子进程的pid分配空间

    my_lock_init("lock.XXXXXX"); /* one lock file for all child */ 
    for (i = 0; i < nchildren; i++)
        pids[i] = child_make(i, listenfd, addrlen); /* parent returns */
    
    signal(SIGINT, sig_int);

    for ( ; ; )
        pause(); /* everything done by children */
}


void sig_int(int signo)
{
    int i;
    void pr_cpu_time(void);

    /* terminate all children */
    for (i = 0; i < nchildren; i++)
        kill(pids[i], SIGTERM);
    while (wait(NULL) > 0)
        ;  /* wait for all children */
    if (errno != ECHILD)
        err_sys("wait error");

    pr_cpu_time();
    exit (0);
}

#include <sys/resource.h>

void pr_cpu_time(void)
{
    double user, sys;
    struct rusage myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
        err_sys("getrusage error");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        err_sys("getrusage error");

    user = (double) myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec / 1000000.0;
    user += (double) childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec / 1000000.0;
    sys = (double) myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec / 1000000.0;
    sys += (double) childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec / 1000000.0;

    printf("\nuser time = %g, sys time = %g\n", user, sys);
}

pid_t child_make(int i, int listenfd, int addrlen)
{
    pid_t pid;
    void child_main(int, int,  int);

    if ((pid = fork()) < 0)
        err_sys("fork error");
    else if (pid > 0)
        return pid; /* parent */

    child_main(i, listenfd, addrlen); /* never returns */
}

void child_main(int i, int listenfd, int addrlen)
{
    int connfd;
    void web_child(int);
    socklen_t clilen;
    struct sockaddr *cliaddr;

    void my_lock_wait();
    void my_lock_release();

    if ((cliaddr = malloc(addrlen)) == NULL)
        err_sys("malloc error");

    printf("child %ld starting\n", (long) getpid());

    for ( ; ; ) {
        clilen = addrlen;
        my_lock_wait(); /* 对文件上锁 */
        if ((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
            err_sys("accept error");
        my_lock_release(); /* 对文件解锁 */

        web_child(connfd); /* process the request */
        if (close(connfd) < 0)
            err_sys("close error");    
    }
}

#define MAXN 16384 /* max # bytes client can request */

void web_child(int sockfd)
{
    int ntowrite, n;
    ssize_t nread;
    char line[MAXLINE], result[MAXLINE];

    int nreply = 4000;
    snprintf(result, sizeof(result), "%d\n", nreply);

    for ( ; ; ) {
        if ((nread = read(sockfd, line, MAXLINE)) < 0)
            err_sys("read error");
        else if (nread == 0)
            return; /* connection closed by other end */
        
        /* line from client specifies # bytes to write back */
        ntowrite = atol(line);
        if ((ntowrite <= 0) || (ntowrite > MAXN))
            err_quit("client request for %d bytes", ntowrite);

        if ((n = writen(sockfd, result, ntowrite)) < 0)
            err_sys("writen error");
    }
}


#include <sys/mman.h>
#include <fcntl.h>

static pthread_mutex_t *mptr; /* actual mutex will be in shared memory */

void my_lock_init(char *pathname) /* pathname实际没用用到，只是直接在原来的函数基础上修改，没有修改该参数 */
{
    int fd;
    pthread_mutexattr_t mattr;

    if ((fd = open("/dev/zero", O_RDWR, 0)) < 0)
        err_sys("open error");

    mptr = mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (close(fd) < 0)
        err_sys("close error");

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED); /* 默认为PTHREAD_PROCESS_PRIVATE，只允许在单个进程内使用 */
    if (pthread_mutex_init(mptr, &mattr) != 0)
        err_sys("pthread_mutex_init error");
    /* 在先前的互斥锁例子中，使用常值PTHREAD_MUTEX_INITIALIZER初始化全局或静态互斥锁变量，然而对于一个存放在共享内存区域中的互斥锁，
     * 我们必须调用一些pthread库函数以告知该函数库：这是一个位于共享内存区中的互斥锁，将用于不同进程之间的上锁 */
}

void my_lock_wait()
{
    if (pthread_mutex_lock(mptr) != 0)
        err_sys("pthread_mutex_lock error");
}

void my_lock_release()
{
    if (pthread_mutex_unlock(mptr) != 0)
        err_sys("pthread_mutex_unlock error");
}