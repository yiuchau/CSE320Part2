#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>


// Assume no input line will be longer than 1024 bytes
#define MAX_INPUT 1024

#define MAX_JOBS 64

int debug = 0;

#define  cse320(fmt, ...) do{if(debug){fprintf(stderr, "" fmt, ##__VA_ARGS__);}}while(0)

struct job {
  pid_t pid;
  int state; //1 for FG, 2 for ST, 3 for BG
  char cmd[64];
}fg;

struct job jobs[MAX_JOBS];
void initJobs();
int addBg(pid_t pid, int state, char* cmd);
int clearBg(pid_t pid);
void addFg(pid_t pid, char* cmd);
void clearFg();


int builtIn(char** argv);
int parseString(char* s, char*** buf, char* delim);

void ExecWrapper(char **myargv, char **envp, char** myPath);
void ExecPath(int inputfd, int outputfd, char**myargv, char** envp, char** myPath);
void Exec(int inputfd, int outputfd, char **myargv, char **envp);

void sigint_handler(int sig);
void sigtstp_handler(int sig);

void reap();

void readSignal(int pid);


int main (int argc, char ** argv, char **envp) {

  int finished = 0;
  char *prompt = "320sh> ";
  char cmd[MAX_INPUT];
  char *path = getenv("PATH");
  char** myPath;
  
  parseString(path, &myPath, ":");

  setvbuf(stdout, NULL, _IONBF, 0);

  if(strcmp(argv[argc - 1], "-d") == 0){
    printf("Debug output.\n");
    debug = 1;
  }

  // Handle ctrl-c
  //signal(SIGINT, sigint_handler);
  // Handle ctrl-z
  //signal(SIGTSTP, sigtstp_handler);

  initJobs();

  errno = 0; //reset errno

  while (!finished) {
    char *cursor;
    char last_char;
    int rv;
    int count;
    char **myargv = NULL;
    int myargc;
    int end = 0;

    cse320( "Errno before reap: %d\n", errno);
    reap();

    char buf[1024];
    if(getcwd(buf, sizeof(buf)) != NULL){
      write(1, "[", 1);
      write(1, buf, strlen(buf));
      write(1,"]", 1);
    }
    else
      cse320( "getcwd error.\n");


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
        end = 1;
        break;
      } if(last_char == 26) {
        write(1, "^z\n", 3);
        end = 1;
        break;
      }else {
	      write(1, &last_char, 1);
      }
    } 
    
    *cursor = '\0';

    if(*cmd == '\n' || end == 1) {
      cse320( "Empty input\n");
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
      cse320( "Parsing arguments..\n");
      myargc = parseString(cmd, &myargv, " \n");

      for(int i = 0; i < myargc; i++) {
        cse320( "[%s]: myargv[%d] = %s\n", myargv[0], i, myargv[i]);
      }

      // Branch on built-in commands that can't pipe or redirect
      // Change echo and other command's builtIn return no.
      if(!builtIn(myargv) || ((strcmp(myargv[0], "echo")) == 0)) {
        // Process execution wrapper which pipes, confirm paths, and redirects before exec.
        ExecWrapper(myargv, envp, myPath);
      }else{
        if(strcmp(myargv[0], "cd") == 0) {
          char *OLDPWD = getenv("OLDPWD");
          char *PWD = getenv("PWD");

          if(!myargv[1]){
            char *home = getenv("HOME");
            cse320( "[cd] (null)\n[envp] HOME: %s\n", home);

            if(chdir(home) == 0){
              cse320( "[cd] chdir %s success.\n", home);
              if(setenv("PWD", home, 1) == 0)
                cse320( "[cd] setenv success: PWD = %s\n", home);
              else
                cse320( "[set] setenv failure: PWD = %s\n", home);

              if(setenv("OLDPWD", PWD, 1) == 0)
                cse320( "[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                cse320( "[cd] setenv failure: OLDPWD = %s\n", PWD);
              OLDPWD = getenv("OLDPWD");
              PWD = getenv("PWD"); 

              cse320( "[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);
            }
            else
              cse320( "[cd] chdir error.\n");
          }
          else if(strcmp(myargv[1], "-") == 0){

            cse320( "[cd] -\n[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);

            if(chdir(OLDPWD) == 0){
              cse320( "[cd] chdir %s success.\n",myargv[1]);

              if(setenv("PWD", OLDPWD, 1) == 0)
                cse320( "[cd] setenv success: PWD = %s\n", OLDPWD);
              else
                cse320( "[set] setenv failure: PWD = %s\n", OLDPWD);

              if(setenv("OLDPWD", PWD, 1) == 0)
                cse320( "[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                cse320( "[cd] setenv failure: OLDPWD = %s\n", PWD);

              OLDPWD = getenv("OLDPWD");
              PWD = getenv("PWD"); 

              cse320( "[envp] OLDPWD: %s \n[envp] PWD: %s\n", OLDPWD, PWD);
            }
            else
              cse320( "[cd] chdir %s error.\n", myargv[1]);
          }else{
            if(chdir(myargv[1]) == 0){

              char buf[1024];
              cse320( "[cd] %s\n[cd] chdir success.\n",myargv[1]);

              if(getcwd(buf, sizeof(buf)) != NULL)
                cse320( "[cd] Current Working Directory is %s\n", buf);
              else
                cse320( "[cd] getcwd error.\n");
              if(setenv("PWD", buf, 1) == 0)
                cse320( "[cd] setenv success: PWD = %s\n", buf);
              else
                cse320( "[set] setenv failure: PWD = %s\n", buf);

              if(setenv("OLDPWD", PWD, 1) == 0)
                cse320( "[cd] setenv success: OLDPWD = %s\n", PWD);
              else
                cse320( "[cd] setenv failure: OLDPWD = %s\n", PWD);
              }else
              cse320( "[cd] chdir %s error.\n", myargv[1]);
          }
        }else
        if(strcmp(myargv[0], "set") == 0) {

          char* var = myargv[1];
          char* value = myargv[3];
          cse320( "[set] environment variable %s to %s\n", var, value);
          if(setenv(var, value, 1) == 0)
            cse320( "[set] setenv success: %s = %s\n", var, value);
          else
            cse320( "[set] setenv failure: %s = %s\n", var, value);
        }
      }

      //free myargv
      cse320( "Freeing myargv..\n");
      free(myargv);
    }
  }
  //free myPath
  cse320( "Freeing myPath..\n");
  free(myPath);
  return 0;
}


void ExecWrapper(char **myargv, char **envp, char** myPath) {
  int pipefd[2];
  int i, inputfd, cmd, nextCmd = 0;

  cse320( "[%s]: examined for pipes.\n", myargv[0]);
  do {
    cmd = nextCmd;

    //cse320( "cmd: %d, nextCmd: %d\n", cmd, nextCmd);
    nextCmd = -1;
    for(i = cmd; (myargv[i] != NULL) && (nextCmd != i); i++) {
      if(strcmp(myargv[i], "|") == 0) {
        myargv[i] = NULL;
        nextCmd = i + 1;
        cse320( "[%s]: piped to [%s] at index %d.\n", myargv[cmd], myargv[nextCmd], i);
      }
    }

    if(nextCmd == -1)
      break;

    pipe(pipefd);
    cse320( "[%s] Attempted execution with pipes...\n", myargv[cmd]);
    ExecPath(inputfd, pipefd[1], &myargv[cmd], envp, myPath);
    close(pipefd[1]);
    inputfd = pipefd[0];

    //cse320( "cmd: %d, nextCmd: %d\n", cmd, nextCmd);
  }while(1);

  cse320( "[%s]: is the only process or final process of pipe.\n", myargv[cmd]);
  ExecPath(inputfd, 1, &myargv[cmd], envp, myPath);
  return;
}

void ExecPath(int inputfd, int outputfd, char**myargv, char** envp, char** myPath) {


  cse320( "[%s]: command is not a built-in command that can not be piped/redirected.\n", myargv[0]);
  cse320( "[%s]: passed through path correction function.\n", myargv[0]);

  if(strcmp(myargv[0], "echo") == 0) {
    cse320( "[echo]: skipped through path finding.\n");
    Exec(inputfd, outputfd, myargv, envp);
    return;
  }
  // Programs:
  // 1. If cmd includes a / character, it is a path, check using stat if file exists, exec file
  // 2. else try all values in path list using stat, then exec
  if(strchr(myargv[0], '/') != NULL) {
    cse320( "[%s]: command is a relative or absolute path.\n", myargv[0]);
    struct stat buf;
    if(stat(myargv[0],&buf) == 0) {
      // file exists

      cse320( "[%s]: File exists, executing..\n", myargv[0]);
      Exec(inputfd, outputfd, myargv, envp);
    }else{
      cse320( "[%s]: File doesn't exist.\n", myargv[0]);
      return;
    }
  } else {
      //cmd is not a path, build path using path list
    cse320( "[%s] command is not a relative or absolute path.\n", myargv[0]);
    int found = 0; // no executions for multiple similar named files in different paths
    int i = 0;
    while(myPath[i] != NULL && found != 1) {
      //check if PATH + program is file, if yes, execute
      struct stat buf;
      char filePath[256];
      strcpy(filePath, myPath[i]);
      strcat(filePath, "/");
      strcat(filePath, myargv[0]);
      //cse320( "Filepath: %s\n", filePath);
      if(stat(filePath,&buf) == 0) {
          // file exists
          //update argv
        myargv[0] = filePath;
        cse320( "[%s]: Path corrected, executing..\n", myargv[0]);
        Exec(inputfd, outputfd, myargv, envp);
        found = 1;
      }
      i++;
    }

    if(!found)
        cse320( "[%s]: command not found in any paths.\n", myargv[0]);
    }

    return;
}



void Exec(int inputfd, int outputfd, char **myargv, char **envp) {
  pid_t pid;
  char *index;


  cse320( "[%s]: (Piped) Inputfd: %d, Outputfd: %d\n", myargv[0], inputfd, outputfd);
  int bg = 0, i = 0;
  // check if process is bg job, last index == '&'
  while(myargv[i] != NULL) {
    i++;
  }
  if(strcmp(myargv[i - 1], "&") == 0) {
    bg = 1;
    myargv[i - 1] = NULL;
  }

  if((pid = fork()) == 0) {
    if(bg)
      setpgid(0, 0);

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
        cse320( "[%s]: myargv[%d]:%s, [%d]:%s\n",myargv[0], i, myargv[i], i + 1, myargv[i + 1]);
        cse320( "[%s]: myargv[%d]: Found <, redirecting input.\n", myargv[0], i);

        if((inputfd = (open(myargv[i + 1], O_RDONLY))) < 0) {
          cse320( "Open error.\n");
          return;
        }

        dup2(inputfd, STDIN_FILENO);
        close(inputfd);

        myargv[i] = NULL;
        myargv[i + 1] = NULL;
        //cse320( "Test: %s, %s\n", myargv[i], myargv[i + 1]);
        i++; //Increment i, skip myargv[i + 1] for other redirections
      }else
      if((index = (strchr(myargv[i], '>'))) != NULL) {
        cse320( "[%s]: myargv[%d]:%s , [%d]:%s\n",myargv[0], i, myargv[i], i + 1, myargv[i + 1]);
        cse320( "[%s]: myargv[%d]: Found >, redirecting output.\n", myargv[0], i);

        if(myargv[i + 1] == NULL) {
          cse320( "No file to redirect output to.\n");
          return;
        }

        if((outputfd = (open(myargv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH))) < 0) {
          cse320( "Open error.\n");
          return;
        }

        dup2(outputfd, STDOUT_FILENO);
        close(outputfd);

        myargv[i] = NULL;
        myargv[i + 1] = NULL;
        //cse320( "[%s] [i]%s, [i+1]%s\n", myargv[i], myargv[i + 1]);
        i++; //Increment i such that loop continues afterwards for further redirections.
      }
    }

    cse320( "[%s]: (Redirected) Inputfd: %d, Outputfd: %d\n", myargv[0], inputfd, outputfd);
    for(int i = 0; myargv[i] != NULL; i++) {
      cse320( "[%s]: Before Execution: myargv[%d]: %s\n", myargv[0], i, myargv[i]);
    }

    if((strcmp(myargv[0], "echo")) == 0) {
      //handle echo with redirection/pipe
      cse320( "[%s]: %s\n", myargv[0], myargv[1]);
      exit(0);
    }

    cse320( "--- Output from execution below ---\n");
    if((execve(myargv[0], myargv, envp)) < 0){
      cse320( "%s command not found.\n", myargv[0]);
      exit(1);
    }
  }

  if(!bg) {
    //process runs in foreground, shell waits
    //add fg job to jobs
    addFg(pid, myargv[0]);
    printf("[%s]: Child %d running in foreground.\n", myargv[0], pid);
    while(pid == fg.pid){
      reap();
      readSignal(pid);
      }
    }
  else {
    //add bg jobs to jobs
    setpgid(pid, pid);
    addBg(pid, 1, myargv[0]);
    printf("[%s]: Child %d running in background.\n", myargv[0], pid);
  }
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
      cse320( "getcwd error.\n");
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

  if(strcmp(argv[0], "jobs") == 0) {
    isBuiltIn = 1;
    for(int i = 0; i < MAX_JOBS; i++) {
      if(jobs[i].pid == 0)
        continue;
      char* stateStr;
      if(jobs[i].state == 2)
        stateStr = "STOPPED";
      else
        stateStr = "RUNNING";
      printf("[%d] (%d) %s %s\n", i, jobs[i].pid, stateStr, jobs[i].cmd);
    }
    cse320( "Last errno: %d\n", errno);
    cse320( "End of jobs..\n");
  }

  if(strcmp(argv[0], "bg") == 0 || strcmp(argv[0], "fg") == 0) {
    int jid = -1, pid = -1;
    isBuiltIn = 1;
    if(argv[1] == NULL) {
      cse320( "Missing argument for pid/jid.\n");
      return isBuiltIn;
    }
    if(argv[1][0] == '%') { //jid
      jid = atoi(argv[1] + sizeof(char));
      if(jobs[jid].pid < 1)
        cse320( "Invalid JID: %d\n", jid);
        pid = jobs[jid].pid;
    }else { //pid
      pid = atoi(argv[1]);
      for(int i = 0; i < MAX_JOBS; i++) {
        if(jobs[i].pid == pid)
          jid = i;
      }

      if(jid == -1) {
        cse320( "Invalid PID: %d\n", pid);
        return isBuiltIn;
      }
      
    }

    cse320( "JID: %%%d, PID: %d\n", jid, pid);

    if(strcmp(argv[0], "bg") == 0) {
      jobs[jid].state = 3;
      kill(pid, SIGCONT);
      cse320( "(%d) resuming..\n", pid);
    }else{
      addFg(pid, jobs[jid].cmd);
      clearBg(pid);
      kill(pid, SIGCONT);
      cse320( "(%d) running in fg..\n", pid);

      while(pid == fg.pid){
      reap();
      readSignal(pid);
      }

    }
  }

  cse320( "[%s] Built-in: %d\n", argv[0], isBuiltIn);
  return isBuiltIn;
}

void initJobs(){
  for(int i = 0; i < MAX_JOBS; i++) {
    jobs[i].pid = 0;
    jobs[i].state = 0;
    jobs[i].cmd[0] = '\0';
  }
}

int addBg(pid_t pid, int state, char* cmd){
  for(int i = 0; i < MAX_JOBS; i++) {
    if(jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      strcpy(jobs[i].cmd, cmd);
      cse320( "Job of pid %d added at index %d.\n",pid, i);
      return 0;
    }
  }
  cse320( "No space in jobs.\n");
  return 1;
}

void addFg(pid_t pid, char* cmd){
  fg.pid = pid;
  fg.state = 1;
  strcpy(fg.cmd, cmd);
  cse320( "FG Job of pid %d added.\n",pid);
  return;
}

void clearFg(){
  fg.pid = 0;
  fg.state = 0;
  fg.cmd[0] = '\0';
  cse320( "FG Job cleared.\n");
  return;
}

int clearBg(pid_t pid){
  for(int i = 0; i < MAX_JOBS; i++) {
    if(jobs[i].pid == pid) {
      jobs[i].pid = 0;
      jobs[i].state = 0;
      jobs[i].cmd[0] = '\0';
      cse320( "Job of pid %d removed at index %d.\n",pid, i);
      return 0;
    }
  }
  cse320( "Not found in jobs.\n");
  return 1;
}


void sigint_handler(int sig){
  cse320( "Signal: Ctrl-C being handled.\n");
  if(sig >= 1) {
    clearFg();
    kill(sig, SIGINT);
  }
  return;
}

void sigtstp_handler(int sig){
  cse320( "Signal: Ctrl-Z being handled.\n");
  if(fg.pid >= 1 || fg.pid != sig) {
    cse320( "Sending SIGTSTP signal to (%d)\n", sig);
    kill(sig, SIGTSTP);
    addBg(sig, 2, fg.cmd);
    clearFg();
  }
  return;
}

void reap() {
  pid_t pid;
  int child_status;
  int saved_errno = errno;
  //cse320( "Reaping..\n");
  if((pid = waitpid(-1, &child_status, WNOHANG)) > 0){
    if(WIFEXITED(child_status)){
      printf("(%d) terminated with exit status %d.\n", pid, WEXITSTATUS(child_status));
      if(fg.pid == pid){
        cse320( "FG Process ended.\n");
        clearFg();
      }
      else{
        cse320( "BG Process ended.\n");
        clearBg(pid);
      }
    }
    else
      printf("Child %d terminated abnormally.\n", pid);

  }
  errno = saved_errno;
  //cse320( "errno is now %d\n", errno);
  return;
}

void readSignal(int pid){
  if(fg.pid != pid)
    return;
  int input;
  read(0, &input, 1);
  if(input == 3){
    sigint_handler(pid);
  }else
  if(input == 26){
    sigtstp_handler(pid);
  }
  return;
}