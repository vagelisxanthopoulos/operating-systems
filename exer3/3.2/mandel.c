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
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000
/* 
 * POSIX thread functions do not return error numbers in errno,
 * but in the actual return value of the function call instead.
 * This macro helps with error reporting in this case.
 */
#define perror_pthread(ret, msg) \
	do { errno = ret; perror(msg); } while (0)

/***************************
 * Compile-time parameters *
 ***************************/
int total_threads;  //arithmos thread pou tha exoume
sem_t* sem;         //pinakas semaphores
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

void compute_and_output_mandel_line(int fd, int line)
{
	/*
	 * A temporary array, used to hold color values for the line being drawn
	 */
	int color_val[x_chars];

	//thread poy typwnei twra
	int curr_thread = line % total_threads;

	//thread poy typwnei amesws meta
	int next_thread = (curr_thread + 1) % total_threads;

	compute_mandel_line(line, color_val);

	if (sem_wait(&sem[curr_thread]) == -1)
	{
		perror("sem_wait failed");
		exit(1);
	}

	output_mandel_line(fd, color_val);

	if (sem_post(&sem[next_thread]) == -1)
	{
		perror("sem_post failed");
		exit(1);
	}
}

void *thread_routine(void *arg)
{
	volatile int *thread_num = arg;
	int line;
	for (line = *thread_num; line < y_chars; line += total_threads) {
		compute_and_output_mandel_line(1, line);
		//output is sent to file descriptor '1', i.e., standard output
	}
	return NULL;	
}

int main(int argc, char** argv)
{
	int ret,i;

	//elegxos swstou input

	if (argc != 2)
	{
		perror("wrong arguments");
		exit(1);
	}

	total_threads = atoi(argv[1]);

      if (total_threads == 0)
	{
		perror("wrong arguments");
		exit(1);
	}

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	//tha exoume osous semaphores einai ta threads

	//oloi tha einai arxikopoimenoi ston 0 
	//ektos apo ton prwto opou tha einai sto 1

	//kathe fora tha typwnei grammi to thread pou exei seira
	//meta tha ksypnaei to epomeno
	//kai meta tha perimenei ston semaphore tou mexri na 
	//na to ksynpnisoun gia na typwsei tin epomeni 
	//grammi tou, diladi tin (prev + total_threads)

	//se oli tin diarkeia tou programmatos oloi
	//oi semaphores tha einai 0 ektos apo enan (auton pou prepei na typwsei)

	//arxikopoiisi semaphores kai error handling
	
	sem = (sem_t*)malloc(total_threads * sizeof(sem_t));
	if (sem == NULL)
	{
		perror("malloc failed");
		exit(1);
	}

	if (sem_init(&sem[0], 0, 1) == -1)
	{
		perror("sem_init failed");
		exit(1);
	}

	for (i=1; i<total_threads; i++)
	{
		if (sem_init(&sem[i], 0, 0) == -1)
		{
			perror("sem_init failed");
			exit(1);
		}
	}

	//ftiaxoume enan pinaka deiktwn se ints
	//pou periexei ta arguments poy tha dexthei i 
	//thread_routine gia to kathe thread

	//epeidi i thread routine dexetai ws orisma
	//deikti me timi ton arithmo tou thread
	//kathe deiktis exei timi ton arithmo tou antistoixou thread

	int thread_arg[total_threads][1];
	for (i=0; i<total_threads; i++) thread_arg[i][0] = i;

	//gennisi threads kai kalesma sinartisewn

	pthread_t t[total_threads];	
	for (i=0; i<total_threads; i++)
	{
		ret = pthread_create(&t[i], NULL, thread_routine, thread_arg[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}
	}

	//termatismos threads	

	for (i=0; i<total_threads; i++)
	{
		ret = pthread_join(t[i], NULL);
		if (ret) perror_pthread(ret, "pthread_join");
	}	

	reset_xterm_color(1);
	return 0;
}
