/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>


/*TODO header file for m(un)map*/
#include <sys/mman.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/

int total_procs;  //arithmos processes
int** acc;         //syssoreutis telikou apotelesmatos
/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void compute_and_accumulate_mandel_line(int fd, int line)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];
	int i;

	compute_mandel_line(line, color_val);
	for (i = 0; i < x_chars; i++) acc[line][i] = color_val[i]; //gemisma pinaka
}

/*
 * Create a shared memory area, usable by all descendants of the calling
 * process.
 */
void *create_shared_memory_area(unsigned int numbytes)
{
	int pages;
	void *addr;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	/* Create a shared, anonymous mapping for this number of pages */
	/* TODO:  */

	addr = mmap(NULL, pages * sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	return addr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
	int pages;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("destroy_shared_memory_area: munmap failed");
		exit(1);
	}
}

void recursion(int proc_num) //routina pou ekteloun ta processes paidia
{
	int line;
	for (line = proc_num; line < y_chars; line += total_procs) {
		compute_and_accumulate_mandel_line(1, line);
		//output is sent to file descriptor '1', i.e., standard output
	}
	return;
}

int main(int argc, char** argv)
{
	int i;

	//elegxos swstou input

	if (argc != 2)
	{
		perror("wrong arguments");
		exit(1);
	}

	total_procs = atoi(argv[1]);

      if (total_procs == 0)
	{
		perror("wrong arguments");
		exit(1);
	}


	unsigned int acc_size = sizeof(int*) * y_chars;
	acc = create_shared_memory_area(acc_size);        //dimiourgia accumulator

	unsigned int line_size = sizeof(int) * x_chars;
	for (i = 0; i < y_chars; i++)
	{
		acc[i] = create_shared_memory_area(line_size);  //dimiouria grammwn
	}

	if (acc == MAP_FAILED)
	{
		perror("mmap failed");
		exit(1);
	}


	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	pid_t p[total_procs];	
	for (i = 0; i < total_procs; i++)
	{
		p[i] = fork();
		if (p[i] < 0) {
			perror("fork");
			exit(1);
		}
		if (p[i] == 0)
		{
			recursion(i);
			exit(1);   
		}
	}

	int status;
	for (i = 0; i < total_procs; i++)
	{
		if (wait(&status) == -1) 
		{
			perror("proccess exit error");
		}
	}

	for (i = 0; i < y_chars; i++) output_mandel_line(1, acc[i]);

	for (i = 0; i < y_chars; i++) destroy_shared_memory_area(acc[i], line_size);   //destroy lines

	destroy_shared_memory_area (acc, acc_size);  //destroy accumulator

	reset_xterm_color(1);
	return 0;
}
