#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void doWrite (int fd, const char *buff, int len)
{
	ssize_t wcnt, idx;
	idx = 0;
	do {
		wcnt = write(fd, buff + idx, len - idx);
		if (wcnt == -1)
		{
			perror("write to output");
			exit(1);
		}
		idx += wcnt;
	} while (idx < len);	
}
