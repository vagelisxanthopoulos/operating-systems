#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3


/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(char name)
{       
	char s[2];
	s[0] = name;
	s[1] = '\0';
	change_pname(s);
	printf("%c: Starting...\n", name);
	printf("%c: Sleeping...\n", name);
	sleep(SLEEP_PROC_SEC);        
	printf("%c: Exiting...\n", name);
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

void recursion (struct tree_node* curr)
{       
	if ((curr->nr_children) == 0) 
	{
		fork_procs(curr->name[0]);
		return;
	}
	change_pname(curr->name);
	printf("%s: Starting...\n", curr->name);
	printf("%s: Waiting...\n", curr->name);
	pid_t p[curr->nr_children];
	int status[curr->nr_children];
	int i;
	for (i = 0; i < curr->nr_children; i++)
	{
		p[i] = fork();
		if (p[i] < 0) {
			perror("fork");
			exit(1);
		}
		if (p[i] == 0)
		{
			recursion(curr->children+i);
			exit(4);
		}
	}
	for (i = 0; i < curr->nr_children; i++)
	{
		p[i] = wait(&status[i]);
		explain_wait_status(p[i], status[i]);
	}
	printf("%s: Exiting...\n", curr->name);
	return;
}

int main(int argc, char** argv)
{
	struct tree_node* root = get_tree_from_file(argv[1]);
	pid_t p;
	int status1;
	p = fork();
	if (p < 0) {
		perror("main: fork");
		exit(1);
	}
	if (p == 0)
	{
		recursion(root);
		exit(4);
	}
	sleep(SLEEP_TREE_SEC);
	show_pstree(p);
	print_tree(root);
	printf("\n\n");
	p = wait(&status1);
	explain_wait_status(p, status1);
	printf("Exiting...\n");
	return 0;
}
