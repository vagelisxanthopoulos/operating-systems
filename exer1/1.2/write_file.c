#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fconc.h"

void write_file(int fd, const char *infile)
{
	int fd2;
	fd2 = open(infile, O_RDONLY);
	if (fd2 == -1){
		perror("open input");
		exit(1);
	}
	char buff[1024];
	ssize_t rcnt=0;
	rcnt = read(fd2, buff, sizeof(buff)-1);
	if (rcnt == 0) /* end-of-file */
		return;
	if (rcnt == -1){ /* error */
		perror("read input");
		exit(1);
	}
	close(fd2);
	buff[rcnt] = '\0';
	doWrite(fd, buff, rcnt+1);	
}
