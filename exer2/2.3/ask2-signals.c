#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#define SLEEP_PROC_SEC  5

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *curr)
{       //den exei bei eidiko block gia tin periptosi fyllou giati se autin tin periptosi
	//apla den tha boume se kamia loopa
	/*
	 * Start
	 */
	printf("PID = %ld, name %s, starting...\n", (long)getpid(), curr->name);
	change_pname(curr->name);

	pid_t p[curr->nr_children];
	int status[curr->nr_children];
	int i;
	for (i = 0; i < curr->nr_children; i++)       //paidia
	{
		p[i] = fork();
		if (p[i] < 0) {
			perror("fork");
			exit(1);
		}
		if (p[i] == 0)
		{
			fork_procs(curr->children+i);
			exit(4);
		}
	}
	/*
	 * Suspend Self
	 */
	int tempstatus;
	pid_t temp;
	for (i=0; i < curr->nr_children; i++)        //perimenoume na stamatisoun ta paidia prin stamatisoume
	{
		temp = waitpid(-1, &tempstatus, WUNTRACED);
		if (!WIFSTOPPED(tempstatus)) {

			fprintf(stderr, "Parent: Child with PID %ld has died unexpectedly!\n", (long)p);
			exit(1);

		}
	}
	// i parapanw loopa einai idia me tin wait_for_ready_children apla xwris to explain_wait_status
	//giati den thelame polla minimata

	//wait_for_ready_children(curr->nr_children);
	raise(SIGSTOP);
	if (curr->nr_children > 0) printf("PID = %ld, name = %s is awake and waiting\n", (long)getpid(), curr->name);
	else 
	{
		printf("PID = %ld, name = %s is awake and sleeping\n", (long)getpid(), curr->name);
		sleep(SLEEP_PROC_SEC);
	}
	/*for (i=0; i < curr->nr_children; i++)       //synexizoume ta paidia
	  {
		  if (kill(p[i], SIGCONT) < 0)
		  {
			  perror("kill");
			  exit(1);
		  }
	  }
	  for (i = 0; i < curr->nr_children; i++)
	  {
		  p[i] = waitpid(p[i], &status[i], 0);
		  explain_wait_status(p[i], status[i]);
	  }*/
	for (i=0; i < curr->nr_children; i++)       //synexizoume ta paidia
	{
		if (kill(p[i], SIGCONT) < 0)
		{
			perror("kill");
			exit(1);
		}
		p[i] = waitpid(p[i], &status[i], 0);
		explain_wait_status(p[i], status[i]);
	}
	printf("%s: Exiting...\n", curr->name);
	return;
}



/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
