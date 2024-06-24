#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int value = 5;

int main(){
	pid_t pid;
	int fd[2];

	if (pipe(fd) == -1) {
		printf("PIPE ERROR\n");
		fprintf(stderr, "Pipe failed");
		return 1;
	}

	pid = fork();

	if (pid == 0) {
		close(fd[0]);
    value += 15;
		write(fd[1], &value, sizeof(value));
		close(fd[1]);
		return 0;
	} 
  else if (pid > 0) {
    int received_valeu;
		close(fd[1]);
		read(fd[0], &received_value, sizeof(received_value));
    close(fd[0]);
    wait(NULL);
		printf("PARENT: value = %d\n", received_value);
		return 0;
	}
}
