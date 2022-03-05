#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fconc.h"
#include <stdbool.h>
#include <string.h>

int main(int argc, char **argv)
{
	if ((argc > 4) | (argc < 3)){
		perror("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]");
		exit(1);
	}
	bool repeat = false;
	bool first = false;
	bool second = false;
	if (argc == 3){
		if (strcmp(argv[1], "fconc.out") == 0){
			repeat = true; 
			first = true;
		}
		else if (strcmp(argv[2], "fconc.out") == 0){
			repeat = true;
			second = true;
		}
	}
	else {
		if (strcmp(argv[1], argv[3]) == 0){
			repeat = true;
			first = true;
		}
		else if (strcmp(argv[2], argv[3]) == 0){
			repeat = true;
			second = true;
		}
	}

	int fd2, oflagstemp, modetemp;

	if (repeat){
		oflagstemp = O_CREAT | O_RDWR | O_TRUNC;
		modetemp = S_IRUSR | S_IWUSR;
		fd2 = open("temp", oflagstemp, modetemp);
		if (argc == 3) write_file(fd2, "fconc.out");
		else write_file(fd2, argv[3]);
	}

	int fd, oflags, mode;
	oflags = O_CREAT | O_WRONLY | O_TRUNC;
	mode = S_IRUSR | S_IWUSR;
	if (argc ==3) fd = open("fconc.out", oflags, mode);
	else fd = open(argv[3], oflags, mode);
	if (fd == -1){
		perror("open output");
		exit(1);
	}
	if (repeat && first){
		write_file(fd, "temp");
		write_file(fd, argv[2]);
		close(fd2);
	}
	else if (repeat && second){
		write_file(fd, argv[1]);
		write_file(fd, "temp");
		close(fd2);
	}
	else {
		write_file(fd, argv[1]);
		write_file(fd, argv[2]);
	}
	close (fd);
	return 0;
}
