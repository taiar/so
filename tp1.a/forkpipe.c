#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


int
main(void) {

  int p[2];
  pipe(p);
  int pid = fork();

  if(pid == 0) {
    char *args[2];
    args[0] = "wc";
    args[1] = 0;
    close(0);
    dup(p[0]);
    close(p[0]);
    close(p[1]);
    execv("/usr/bin/wc", args);
    printf("execv returnex\n");
  }

}
