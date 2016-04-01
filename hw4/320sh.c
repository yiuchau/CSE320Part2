#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>

// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024


int builtIn(char** argv);
int parseString(char* s, char*** buf, char* delim);



void ExecWrapper(char **myargv, char **envp, char** myPath);
void ExecPath(int inputfd, int outputfd, char**myargv, char** envp, char** myPath);
void Exec(int inputfd, int outputfd, char **myargv, char **envp);

int main (int argc, char ** argv, char **envp) {

  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];
  char *path = getenv("PATH");
  char** myPath;
  
  parseString(path, &myPath, ":");

  setvbuf(stdout, NULL, _IONBF, 0);


  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;
    char **myargv = NULL;
    int myargc;

    char buf[1024];
    if(getcwd(buf, sizeof(buf)) != NULL){
      write(1, "[", 1);
      write(1, buf, strlen(buf));
      write(1,"]", 1);
    }
    else
      printf("getcwd error.\n");


    // Print the prompt
    rv = write(1, prompt, strlen(prompt));
    if (!rv) { 
      finished = 1;
      break;
    }
    
    // read and parse the input
    for(rv = 1, count = 0, cursor = cmd, last_char = 1; 
          rv && (++count < (MAX_INPUT-1)) && (last_char != '\n');
	         cursor++) { 
      rv = read(0, cursor, 1);
      last_char = *cursor;
      // input is ^C
      if(last_char == 3) {
        write(1, "^c\n", 3);
        finished = 1;
        break;
      } else {
	      write(1, &last_char, 1);
      }
    } 
    
    *cursor = '\0';

    if(*cmd == '\n') {
      printf("Empty input\n");
      continue;
    }

    if (!rv) { 
      finished = 1;
      break;
    }


    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now
    // write(1, cmd, strnlen(cmd, MAX_INPUT));

    // parse envp variables
    // parse cmd on spaces

    if(1) {
      printf("Parsing arguments..\n");
      myargc = parseString(cmd, &myargv, " \n");

      for(int i = 0; i < myargc; i++) {
        printf("[%s]: myargv[%d] = %s\n", myargv[0], i, myargv[i]);
      }

      // Branch on built-in commands that can't pipe or redirect
      // Change echo and other command's builtIn return no.
      if(!builtIn(myargv) || ((strcmp(myargv[0], "echo")) == 0)) {
        // Process execution wrapper which pipes, confirm paths, and redirects before exec.
        ExecWrapper(myargv, envp, myPath);
      }else{
        // Handle built-in commands here to remain inscope for envp
        if(strcmp(myargv[0], "cd") == 0) {
          char *OLDPWD = getenv("OLDPWD");
          char *PWD = getenv("PWD");

          if(!myargv[1]){
            char *home = getenv("HOME");
            printf("[cd] (null)\n[envp] HOME: %s\n", home);

            if(chdir(home) == 0){
              printf("[cd] chdir %s success.\n", home);
              if(setenv("PWD", home, 1) == 0)
                printf("[cd] setenv success: PWD = %s\n", home);
              else
                printf("[set] setenv failure: PWD = %s\n", home);

              if(setenv("OLDPWD", PWD, 1) == 0)
                printf("[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                printf("[cd] setenv failure: OLDPWD = %s\n", PWD);
              OLDPWD = getenv("OLDPWD");
              PWD = getenv("PWD"); 

              printf("[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);
            }
            else
              printf("[cd] chdir error.\n");
          }
          else if(strcmp(myargv[1], "-") == 0){

            printf("[cd] -\n[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);

            if(chdir(OLDPWD) == 0){
              printf("[cd] chdir %s success.\n",myargv[1]);

              if(setenv("PWD", OLDPWD, 1) == 0)
                printf("[cd] setenv success: PWD = %s\n", OLDPWD);
              else
                printf("[set] setenv failure: PWD = %s\n", OLDPWD);

              if(setenv("OLDPWD", PWD, 1) == 0)
                printf("[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                printf("[cd] setenv failure: OLDPWD = %s\n", PWD);

              OLDPWD = getenv("OLDPWD");
              PWD = getenv("PWD"); 

              printf("[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);
            }
            else
              printf("[cd] chdir %s error.\n", myargv[1]);
          }else{
            if(chdir(myargv[1]) == 0){

              char buf[1024];
              printf("[cd] %s\n[cd] chdir success.\n",myargv[1]);

              if(getcwd(buf, sizeof(buf)) != NULL)
                printf("[cd] Current Working Directory is %s\n", buf);
              else
                printf("[cd] getcwd error.\n");
              if(setenv("PWD", buf, 1) == 0)
                printf("[cd] setenv success: PWD = %s\n", buf);
              else
                printf("[set] setenv failure: PWD = %s\n", buf);

              if(setenv("OLDPWD", PWD, 1) == 0)
                printf("[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                printf("[cd] setenv failure: OLDPWD = %s\n", PWD);
              }else
              printf("[cd] chdir %s error.\n", myargv[1]);
          }
        }else
        if(strcmp(myargv[0], "set") == 0) {

          char* var = myargv[1];
          char* value = myargv[3];
          printf("[set] environment variable %s to %s\n", var, value);
          if(setenv(var, value, 1) == 0)
            printf("[set] setenv success: %s = %s\n", var, value);
          else
            printf("[set] setenv failure: %s = %s\n", var, value);
        }
      }

      //free myargv
      printf("Freeing myargv..\n");
      free(myargv);
    }
  }
  //free myPath
  printf("Freeing myPath..\n");
  free(myPath);
  return 0;
}


void ExecWrapper(char **myargv, char **envp, char** myPath) {
  int pipefd[2];
  int i, inputfd, cmd, nextCmd = 0;

  printf("[%s] examined for pipes.\n", myargv[0]);
  do {
    cmd = nextCmd;

    //printf("cmd: %d, nextCmd: %d\n", cmd, nextCmd);
    nextCmd = -1;
    for(i = cmd; (myargv[i] != NULL) && (nextCmd != i); i++) {
      if(strcmp(myargv[i], "|") == 0) {
        myargv[i] = NULL;
        nextCmd = i + 1;
        printf("[%s] piped to [%s] at index %d.\n", myargv[cmd], myargv[nextCmd], i);
      }
    }

    if(nextCmd == -1)
      break;

    pipe(pipefd);
    printf("[%s] Attempted execution with pipes...\n", myargv[cmd]);
    ExecPath(inputfd, pipefd[1], &myargv[cmd], envp, myPath);
    close(pipefd[1]);
    inputfd = pipefd[0];

    //printf("cmd: %d, nextCmd: %d\n", cmd, nextCmd);
  }while(1);

  printf("[%s] is the only process or final process of pipe.\n", myargv[cmd]);
  ExecPath(inputfd, 1, &myargv[cmd], envp, myPath);
  return;
}

void ExecPath(int inputfd, int outputfd, char**myargv, char** envp, char** myPath) {


  printf("[%s] command is not a built-in command that can not be piped/redirected.\n", myargv[0]);
  printf("[%s] passed through path correction function.\n", myargv[0]);

  if(strcmp(myargv[0], "echo") == 0) {
    printf("[echo] skipped through path finding.\n");
    Exec(inputfd, outputfd, myargv, envp);
    return;
  }
  // Programs:
  // 1. If cmd includes a / character, it is a path, check using stat if file exists, exec file
  // 2. else try all values in path list using stat, then exec
  if(strchr(myargv[0], '/') != NULL) {
    printf("[%s] command is a relative or absolute path.\n", myargv[0]);
    struct stat buf;
    if(stat(myargv[0],&buf) == 0) {
      // file exists

      printf("[%s] File exists, executing..\n", myargv[0]);
      Exec(inputfd, outputfd, myargv, envp);
    }
  } else {
      //cmd is not a path, build path using path list
    printf("[%s] command is not a relative or absolute path.\n", myargv[0]);
    int found = 0; // no executions for multiple similar named files in different paths
    int i = 0;
    while(myPath[i] != NULL && found != 1) {
      //check if PATH + program is file, if yes, execute
      struct stat buf;
      char filePath[256];
      strcpy(filePath, myPath[i]);
      strcat(filePath, "/");
      strcat(filePath, myargv[0]);
      printf("Filepath: %s\n", filePath);
      if(stat(filePath,&buf) == 0) {
          // file exists
          //update argv
        myargv[0] = filePath;
        printf("[%s] Path corrected, executing..\n", myargv[0]);
        Exec(inputfd, outputfd, myargv, envp);
        found = 1;
      }
      i++;
    }

    if(!found)
        printf("[%s] command not found in any paths.\n", myargv[0]);
    }

    return;
}



void Exec(int inputfd, int outputfd, char **myargv, char **envp) {
  pid_t pid;
  int child_status;
  char *index;


  printf("[%s]: (Piped) Inputfd: %d, Outputfd: %d\n", myargv[0], inputfd, outputfd);

  if((pid = fork()) == 0) {

    if(inputfd != 0) {
      dup2(inputfd, 0);
      close(inputfd);
    }
    if(outputfd != 1) {
      dup2(outputfd, 1);
      close(outputfd);
    }
    // redirected fds override pipes
    for(int i = 0; myargv[i] != NULL; i++) {
      if((index = (strchr(myargv[i], '<'))) != NULL) {     
        printf("[%s]: myargv[%d]:%s, [%d]:%s\n",myargv[0], i, myargv[i], i + 1, myargv[i + 1]);
        printf("[%s]: myargv[%d]: Found <, redirecting input.\n", myargv[0], i);

        if((inputfd = (open(myargv[i + 1], O_RDONLY))) < 0) {
          printf("Open error.\n");
          return;
        }

        dup2(inputfd, STDIN_FILENO);
        close(inputfd);

        myargv[i] = NULL;
        myargv[i + 1] = NULL;
        printf("Test: %s, %s\n", myargv[i], myargv[i + 1]);
        i++; //Increment i, skip myargv[i + 1] for other redirections
      }else
      if((index = (strchr(myargv[i], '>'))) != NULL) {
        printf("[%s]: myargv[%d]:%s , [%d]:%s\n",myargv[0], i, myargv[i], i + 1, myargv[i + 1]);
        printf("[%s]: myargv[%d]: Found >, redirecting output.\n", myargv[0], i);

        if(myargv[i + 1] == NULL) {
          printf("No file to redirect output to.\n");
          return;
        }

        if((outputfd = (open(myargv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH))) < 0) {
          printf("Open error.\n");
          return;
        }

        dup2(outputfd, STDOUT_FILENO);
        close(outputfd);

        myargv[i] = NULL;
        myargv[i + 1] = NULL;
        //printf("[%s] [i]%s, [i+1]%s\n", myargv[i], myargv[i + 1]);
        i++; //Increment i such that loop continues afterwards for further redirections.
      }
    }

    printf("[%s]: (Redirected) Inputfd: %d, Outputfd: %d\n", myargv[0], inputfd, outputfd);
    for(int i = 0; myargv[i] != NULL; i++) {
      printf("[%s]: Before Execution: myargv[%d]: %s\n", myargv[0], i, myargv[i]);
    }

    if((strcmp(myargv[0], "echo")) == 0) {
      //handle echo with redirection/pipe
      printf("[%s]%s\n", myargv[0], myargv[1]);
      exit(0);
  }

    if((execve(myargv[0], myargv, envp)) < 0){
      printf("%s command not found.\n", myargv[0]);
    }
  } 

  waitpid(pid, &child_status, 0);
  if(WIFEXITED(child_status))
    printf("Child %d terminated with exit status %d.\n", pid, WEXITSTATUS(child_status));
  else
    printf("Child %d terminated abnormally.\n", pid);
  return;
}

int parseString(char* s, char*** buf, char* delim) {
  char *token;
  char **buf2 = calloc(1, sizeof(char*));
  int count = 1;

  token = strtok(s, delim);

  while (token != NULL) {
    buf2[count - 1] = token;
    count++;
    buf2 = realloc(buf2, count * sizeof(char*));
    token = strtok(NULL, delim);
  }

  buf2[count - 1] = NULL;
  *buf = buf2;
  return count - 1; // last arg is a null pointer, otherwise segfault path count
}

int builtIn(char** argv) {
  int isBuiltIn = 0;
  if(strcmp(argv[0], "exit") == 0) {
    free(argv);
    exit(0);
    }

  if(strcmp(argv[0], "pwd") == 0) {
    isBuiltIn = 1;
    char buf[1024];
    if(getcwd(buf, sizeof(buf)) != NULL)
      printf("%s: Current Working Directory is %s\n", argv[0], buf);
    else
      printf("getcwd error.\n");
    }

  if(strcmp(argv[0], "cd") == 0) {
    isBuiltIn = 1;
  }

  if(strcmp(argv[0], "echo") == 0) {
    isBuiltIn = 1;
  }

  if(strcmp(argv[0], "set") == 0) {
    isBuiltIn = 1;
  }

  if(strcmp(argv[0], "help") == 0) {
    isBuiltIn = 1;
  }

  printf("Built-in: %d\n", isBuiltIn);
  return isBuiltIn;
}