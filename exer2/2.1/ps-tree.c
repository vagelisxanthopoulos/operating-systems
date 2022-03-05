#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(char name, int b)
{       
        char s[2];
        s[0] = name;
        s[1] = '\0';
        change_pname(s);
        printf("%c: Starting...\n", name);
        printf("%c: Sleeping...\n", name);
        sleep(SLEEP_PROC_SEC);        
        printf("%c: Exiting...\n", name);
        exit(b);
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
int main(void)
{
     pid_t p, q, r, s;
        int status1, status2, status3, status4;
        /* Fork root of process tree */
        p = fork();
        if (p < 0) {
                perror("main: fork");
                exit(1);
        }
        if (p == 0) {
                /* Child A*/
                change_pname("A");
                printf("A: Starting...\n");
                printf("A: Waiting...\n");
                q = fork();
                if (q < 0) {
                        perror("main: fork");
                        exit(1);
                }
                if (q == 0) {
			/* Child B*/
                        change_pname("B");
                        printf("B: Starting...\n");
                        printf("B: Waiting...\n");
                        s = fork();
                        if (s < 0) {
                                perror("main: fork");
                                exit(1);
                        }
                        if (s == 0) {
                                /* Child D*/
                                fork_procs('D', 13);
                                exit(1);
                        }
                        s = wait(&status4);
                        explain_wait_status(s, status4);
                        printf("B: Exiting...\n");
                        exit(19);
                }
                r = fork();
                if (r < 0) {
                        perror("main: fork");
                        exit(1);
                }
                if (r == 0) {
                        /* Child C*/
                        fork_procs('C', 17);
                        exit(1);
                }
                q = wait(&status2);
                r = wait(&status3);
                explain_wait_status(q, status2);
                explain_wait_status(r, status3);
                printf("A: Exiting...\n");
                exit(16);
        }

   
        /*
         * Father
         */
        /* for ask2-signals */
        /* wait_for_ready_children(1); */

        /* for ask2-{fork, tree} */
        sleep(SLEEP_TREE_SEC);

        /* Print the process tree root at pid */
        show_pstree(p);

        /* for ask2-signals */
        /* kill(pid, SIGCONT); */

        /* Wait for the root of the process tree to terminate */
        p = wait(&status1);
        explain_wait_status(p, status1);
        printf("Exiting...\n");
        return 0;
}
