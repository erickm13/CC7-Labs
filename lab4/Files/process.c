#include <stdio.h>
#include <unistd.h>

int main(void)
{
  printf("Laboratorio 4: Creacion y Manipulacion de Procesos\n");
  printf("\n");
  printf(">>>> 1. Creating a New Process <<<<\n");
  pid_t pid = fork();
    
  if(pid < 0){
    perror("fork");
    return 1;
  }

  if(pid != 0){
    printf("Parent Process: PID = %d\n", getpid());
    sleep(1);
  }else {
    printf("Child Process: PID = %d Parent PID = %d\n", getpid(), getppid());
    sleep(1);
  }


  printf(">>>> 2. Syncronized  Parent ");




  return 0;
}
