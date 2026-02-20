#include <stdio.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <sys/shm.h>

int main(void)
{
  printf("Laboratorio 4: Creacion y Manipulacion de Procesos\n");
  printf("\n");
  printf(">>>> 1. Creating a New Process <<<<\n");
  fflush(stdout);
  pid_t pid = fork();
    
  if(pid < 0){
    perror("fork");
    return 1;
  }

  if(pid != 0){
    printf("Parent Process: PID = %d\n", getpid());
    sleep(1);
    wait(NULL);
  }else {
    printf("Child Process: PID = %d Parent PID = %d\n", getpid(), getppid());
    sleep(1);
    return 0;
  }

  printf(">>>> 2. Synchronized Parent (waitpid) <<<<\n");
  fflush(stdout);
  pid_t pid2 = fork();
  
  if(pid2 < 0){
    perror("fork");
    return 1;
  }

  if(pid2 == 0){
    printf("Child Process: PID = %d, Parent PID = %d\n", getpid(), getppid());
    return 0;
  } else {
    waitpid(pid2, NULL, 0);
    printf("Parent Process: Child has finished execution\n");
  }

  printf(">>>> 3. Inter-Process Communication Using Pipes <<<<\n");
  fflush(stdout);
  
  int pipefd[2];
  char buffer[100];
  
  if(pipe(pipefd) == -1){
    perror("pipe");
    return 1;
  }
  
  pid_t pid3 = fork();
  
  if(pid3 < 0){
    perror("fork");
    return 1;
  }
  
  if(pid3 == 0){
    close(pipefd[1]);
    read(pipefd[0], buffer, sizeof(buffer));
    printf("Child Process: Received \"%s\"\n", buffer);
    close(pipefd[0]);
    return 0;
  } else {
    close(pipefd[0]);
    char *message = "Hello from Parent";
    write(pipefd[1], message, strlen(message) + 1);
    close(pipefd[1]);
    printf("Parent Process: Writing \"%s\"\n", message);
    waitpid(pid3, NULL, 0);
  }

  printf(">>>> 4. Creating Multiple Child Processes <<<<\n");
  fflush(stdout);

  printf("Parent Process: PID = %d\n", getpid());
  fflush(stdout);
  
  for(int i = 1; i <= 3; i++){
    pid_t child_pid = fork();
    
    if(child_pid < 0){
      perror("fork");
      return 1;
    }
    
    if(child_pid == 0){
      printf("Child %d: PID = %d, Parent PID = %d\n", i, getpid(), getppid());
      return 0;
    }
  }

  for(int i = 0; i < 3; i++){
    wait(NULL);
  }

  printf(">>>> 5. Shared Memory <<<<\n");
  fflush(stdout);

  int shm_id;
  char *shm_ptr;
  char data[] = "Shared Memory Example";
  
  shm_id = shmget(IPC_PRIVATE, sizeof(data), IPC_CREAT | 0666);
  if(shm_id < 0){
    perror("shmget");
    return 1;
  }
  fflush(stdout);
  pid_t pid5 = fork();
  
  if(pid5 < 0){
    perror("fork");
    return 1;
  }
  
  if(pid5 == 0){
    shm_ptr = (char*)shmat(shm_id, NULL, SHM_RDONLY);
    if(shm_ptr == (char*)-1){
      perror("shmat");
      return 1;
    }
    printf("Child Process: Read \"%s\"\n", shm_ptr);
    return 0;
  } else {
    shm_ptr = (char*)shmat(shm_id, NULL, 0);
    if(shm_ptr == (char*)-1){
      perror("shmat");
      return 1;
    }
    strcpy(shm_ptr, data);
    printf("Parent Process: Writing \"%s\"\n", data);
    waitpid(pid5, NULL, 0);

  }

  return 0;
}
