#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define  BUFFER_SIZE  25
#define  READ_END  0
#define  WRITE_END  1

int main(void) {
  pid_t pid;
  int fdA[2], fdB[2];
  char write_msg[BUFFER_SIZE], read_msg[BUFFER_SIZE];
  int count;

  if (pipe(fdA) == -1 || pipe(fdB) == -1) {
    printf("PIPE ERROR\n");
    fprintf(stderr, "Pipe failed");
    return 1;
  }

  pid = fork();

  if (pid < 0) {
    printf("FORK ERROR\n");
    fprintf(stderr, "Fork failed");
    return 1;
  }
  
  if (pid == 0) {
    count = 1000;
    close(fdA[WRITE_END]);
    close(fdB[READ_END]);

    for (int i = 0; i < 5; i++) {
        read(fdA[READ_END], read_msg, BUFFER_SIZE);
        printf("child got message: %s\n", read_msg);
        sprintf(write_msg, "child %d", count++);
        write(fdB[WRITE_END], write_msg, strlen(write_msg) + 1);
        sleep(1);
    }

    close(fdA[READ_END]);
    close(fdB[WRITE_END]);
  } 
  else if (pid > 0) {
    count = 0;
    close(fdA[READ_END]);
    close(fdB[WRITE_END]);

    for (int i = 0; i < 5; i++) {
        sprintf(write_msg, "parent %d", count++);
        write(fdA[WRITE_END], write_msg, strlen(write_msg) + 1);
        sleep(1);
        read(fdB[READ_END], read_msg, BUFFER_SIZE);
        printf("parent got message: %s\n", read_msg);
    }

    close(fdA[WRITE_END]);
    close(fdB[READ_END]);
  }
  
  return 0;
}

