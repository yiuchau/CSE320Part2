#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>

// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024


int builtIn(char** argv);
int parseString(char* s, char*** buf, char* delim);

void Exec(char **myargv, char **envp);

int main (int argc, char ** argv, char **envp) {

  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];

  char *path = getenv("PATH");
  char** myPath;
  int pathCount;
  pathCount = parseString(path, &myPath, ":");


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
        write(1, "^c", 2);
      } else {
	      write(1, &last_char, 1);
      }
    } 
    
    *cursor = '\0';

    if (!rv) { 
      finished = 1;
      break;
    }


    // Execute the command, handling built-in commands separately 
    // Just echo the command line for now
    // write(1, cmd, strnlen(cmd, MAX_INPUT));

    // parse envp variables
    // parse cmd on spaces
    myargc = parseString(cmd, &myargv, " \n");
    for(int i = 0; i < myargc; i++) {
      printf("myargv[%d] = %s\n", i, myargv[i]);
    }

    // check if built-in cmd or program
    if(!builtIn(myargv)) {
      printf("%s command is not a built-in command.\n", myargv[0]);
    // Programs:
    // 1. If cmd includes a / character, it is a path, check using stat if file exists, exec file
    // 2. else try all values in path list using stat, then exec
      if(strchr(myargv[0], '/') != NULL) {
        printf("%s command is a relative or absolute path.\n", myargv[0]);
        struct stat buf;
        if(stat(myargv[0],&buf) == 0) {
          // file exists
          Exec(myargv, envp);
        }
      } else {
        //cmd is not a path, build path using path list
        int found = 0;

        for(int i = 0; i < pathCount; i++) {
          //check if PATH + program is file, if yes, execute
          struct stat buf;
          char filePath[256];
          strcpy(filePath, myPath[i]);
          strcat(filePath, "/");
          strcat(filePath, myargv[0]);
          printf("Filepath: %s\n", filePath);
          if(stat(filePath,&buf) == 0) {
            // file exists
            printf("File exists, executing..\n");
            //update argv
            myargv[0] = filePath;
            Exec(myargv, envp);
            found = 1;
          }
        }

        if(!found)
          printf("%s command not found.\n", myargv[0]);
      }
    }else{
      if(strcmp(myargv[0], "cd") == 0) {
        char *OLDPWD = getenv("OLDPWD");
        char *PWD = getenv("PWD");

        if(!myargv[1]){
          printf("myargv[1] is null: %s\n", myargv[1]);
          char *home = getenv("HOME");
          printf("HOME: %s\n", home);
          if(chdir(home) == 0){
            printf("chdir %s success.\n", home);
            setenv("PWD", home, 1);
            setenv("OLDPWD", PWD, 1);
          }
          else
            printf("chdir %s error.\n", myargv[1]);
        }
        else if(strcmp(myargv[1], "-") == 0){
          printf("1. OLDPWD: %s, PWD: %s\n", OLDPWD, PWD);
          if(chdir(OLDPWD) == 0){
            printf("chdir %s success.\n",myargv[1]);
            setenv("PWD", OLDPWD, 1);
            setenv("OLDPWD", PWD, 1);
            OLDPWD = getenv("OLDPWD");
            PWD = getenv("PWD"); 
            printf("2. OLDPWD: %s, PWD: %s\n", OLDPWD, PWD);
          }
          else
            printf("chdir %s error.\n", myargv[1]);
        }else{
          if(chdir(myargv[1]) == 0){
            char buf[1024];
            printf("chdir %s success.\n",myargv[1]);
            if(getcwd(buf, sizeof(buf)) != NULL)
              printf("%s: Current Working Directory is %s\n", myargv[0], buf);
            else
              printf("getcwd error.\n");
            setenv("PWD", buf, 1);
            setenv("OLDPWD", PWD, 1);
          }
          else
            printf("chdir %s error.\n", myargv[1]);
        }
      }else
      if(strcmp(myargv[0], "echo") == 0) {

      }else
      if(strcmp(myargv[0], "set") == 0) {
        printf("setting environment variable..\n");
        char* var = myargv[1];
        char* value = myargv[3];
        if(setenv(var, value, 1) == 0)
          printf("setenv success: %s = %s\n", var, value);
        else
          printf("setenv failure: %s = %s\n", var, value);
      }
    }

    //free myargv
    free(myargv);

  }

  return 0;
}

void Exec(char **myargv, char **envp) {
  pid_t pid;
  int child_status;
  if((pid = fork()) == 0) {
    if((execve(myargv[0], myargv, envp)) < 0){
      printf("%s command not found.\n", myargv[0]);
      exit(1);
    }
  } 

  waitpid(pid, &child_status, 0);
  if(WIFEXITED(child_status))
    printf("Child %d terminated with exit status %d.\n", pid, WEXITSTATUS(child_status));
  else
    printf("Child %dterminated abnormally.\n", pid);
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
  return count - 1; // last arg is a null pointer
}

int builtIn(char** argv) {
  int isBuiltIn = 0;
  if(strcmp(argv[0], "exit") == 0) {
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

  return isBuiltIn;
}