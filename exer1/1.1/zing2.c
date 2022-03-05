#include <stdio.h>
#include <unistd.h>

void zing (void) 
{
	char *username=getlogin();
	printf("Good morning, %s!\n", username);
}
