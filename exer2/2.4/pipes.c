#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

//#define SLEEP_PROC_SEC  5
//#define SLEEP_TREE_SEC  2




void recursion (struct tree_node* curr, int fdwrite)       //fdwrite: file descriptor gia na grapsei sto father process
{      
	if (curr->nr_children == 0)
	{
		change_pname(curr->name);
		//sleep(SLEEP_PROC_SEC);
		int res = atoi(curr->name);      //metatropi onomatos se integer
		if (write(fdwrite, &res, sizeof(res)) != sizeof(res))
		{
			perror("write to pipe");
			exit(1);
		}
		return;
	} 
	change_pname(curr->name);
	//printf("%s: Starting...\n", curr->name);
	pid_t p[2];
	int status[2];
	int pfd[2];
	int value[2];
	int i;
	if (pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}
	for (i = 0; i < 2; i++)
	{
		p[i] = fork();
		if (p[i] < 0) {
			perror("fork");
			exit(1);
		}
		if (p[i] == 0)
		{
			recursion(curr->children+i, pfd[1]);
			close(pfd[0]);
			close(pfd[1]);
			exit(4);
		}
	}
	for (i = 0; i < 2; i++)
	{
		p[i] = wait(&status[i]);
		//explain_wait_status(p[i], status[i]);
	}
	int rcnt = 0;
	int temp;
	while (rcnt < sizeof(value))
	{	
		temp = read(pfd[0], value, sizeof(value));
		if (temp == 0) break;
		if (temp == -1)
		{
			perror("read from pipe");
			exit(1);
		}
		rcnt = rcnt + temp;
	}
	int res;
	if (curr->name[0] == '*') res = value[0] * value[1];
	else res = value[0] + value[1];
	if (write(fdwrite, &res, sizeof(res)) != sizeof(res))
	{
		perror("write to pipe");
		exit(1);
	}
	//printf("%s: Exiting...\n", curr->name);
	return;
}

int main(int argc, char** argv)
{
	struct tree_node* root = get_tree_from_file(argv[1]);
	pid_t p;
	int status1;
	int pfd[2];
	if (pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}
	p = fork();
	if (p < 0) {
		perror("main: fork");
		exit(1);
	}
	if (p == 0)
	{
		recursion(root, pfd[1]);
		close(pfd[0]);
		close(pfd[1]);
		exit(4);
	}
	//sleep(SLEEP_TREE_SEC);
	//sshow_pstree(p);
	p = wait(&status1);
	int res;
	if (read(pfd[0], &res, sizeof(res)) != sizeof(res)) {
		perror("read from pipe");
		return 1;
	}
	printf("%d\n", res);
	return 0;
}

