#include "apue.h"
#include <fcntl.h>

int log_to_stderr = 1;

int main(int argc, char *argv[])
{
	int val;

	if (argc != 2)
		err_quit("usage: apue_fctl.out <descriptor#>" );

	if ((val = fcntl(atoi(argv[1]), F_GETFL, 0)) < 0) //获取文件状态标志
		err_sys("fcntl error for fd %d", atoi(argv[1]));					

	switch (val & O_ACCMODE) { //O_ACCMODE 屏蔽字
		case O_RDONLY:
			printf("read only");
			break;

		case O_WRONLY:
			printf("wtite only");
			break;

		case O_RDWR:
			printf("read write");
			break;

		default:
			err_dump("unknown access mode");
	}

	if (val & O_APPEND)
		printf(", append");
	if (val & O_NONBLOCK)
		printf(", nonblocking");
	if (val & O_SYNC)
		printf(", synchronous writes");

#if !defined(_POSIX_C_SOURCE) && defined(O__FSYNC) && (O_FSYNC != O_SYNC)
	if (val & O_FSYNC)
		printf(", synchronous writes");
#endif

	putchar('\n');
	exit(0);
}
