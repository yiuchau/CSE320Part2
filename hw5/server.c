#include "csapp.h"
//#include "cse320.h"

int listenfd;

void echo(int connfd);
void *loginThread(void *vargp);
void command(void);


struct user {
    //login_time
    char * username;
    int connfd;
    //ipaddr
    struct sockaddr_storage *sockaddr;
    struct user *next;
}*users_head, *users_tail;


#define Usage printf("./server [-h|-v] PORT_NUMBER MOTD\n \
-h         Displays help menu & returns EXIT_SUCCESS\n \
-v         Vebose pring all incoming and outcoming protcol verbs & content. \
PORT_NUMBER Port number to listen on \
MOTD       Message to display to the client when they connect.\n");

void sigintHandler(int sig){

    fprintf(stderr, "\x1B[1;31m :SIGINT CAUGHT Close(%d)\n", listenfd);
    Close(listenfd);
    exit(0);
}

int main(int argc, char **argv) 
{
    int vflag = 0;
    //*connfdp
    char opt;
    socklen_t clientlen;
    struct sockaddr_storage * clientaddr;
    pthread_t tid;
    struct user *newUser;
    fd_set read_set, ready_set;

    if (argc < 3) {
        //print usage statement
        Usage;
        exit(0);
    }

    while((opt = getopt(argc, argv, "hv")) != -1) {
        switch(opt) {
            case 'h':
                fprintf(stderr, "The help menu was selected\n");
                Usage;
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                fprintf(stderr, "Verbosity flag provided\n");
                vflag++;
                break;
            case '?':
                /* Let this case fall down to default;
                 * handled during bad option.
                 */
            default:
                fprintf(stderr,"A bad option was provided.\n");
                Usage;
                exit(EXIT_FAILURE);
                break;
        }
    }

    //changed to global for sig handling
    listenfd = Open_listenfd(atoi(argv[argc - 2]));
    //handle errors
    fprintf(stdout, "\x1B[1;34mCurrently listening on port %d\n", atoi(argv[argc - 2]));

    //signal handler to close socket upon ctrl-c
    Signal(SIGINT, sigintHandler);


    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(listenfd, &read_set);     /* Add listenfd to read set */

    while (1) {
        //create com thread
        if(users_head != NULL) {
            //saved thread for comm thread is closed
            //Pthread_create(&tid, NULL, commThread, NULL);
        }
        ready_set = read_set;
        Select(listenfd+1, &ready_set, NULL, NULL, NULL); 
        if (FD_ISSET(STDIN_FILENO, &ready_set)) 
            fprintf(stdout, "\x1B[1;34mReading command line from stdin.\n");
            fflush(stdout);
            command(); /* Read command line from stdin */
        if (FD_ISSET(listenfd, &ready_set)) { 
            fprintf(stdout, "\x1B[1;34mReading from listenfd: New connection.\n");
            fflush(stdout);
            clientlen = sizeof(struct sockaddr_storage);
            //connfdp = Malloc(sizeof(int)); 
            //*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
            newUser = Malloc(sizeof(struct user));
            //setip
            newUser->connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
            
            newUser->sockaddr = clientaddr;

            Pthread_create(&tid, NULL, loginThread, newUser);
            //Free(connfdp);
    }
    }
}

/* Thread routine */
void *loginThread(void *vargp) 
{  
    //int connfd = *((int *)vargp);
    struct user *newUser = ((struct user *)vargp);
    struct user *ptr = users_head;
    char username[MAXLINE];
    //struct user *usersPtr = users_head;
    Pthread_detach(pthread_self()); 
    //Free(vargp); 

    fprintf(stderr, "\x1B[1;34mNew login thread created for %d.\n", newUser->connfd);

    // Attempt to login,
    Read(newUser->connfd, username, MAXLINE);


    fprintf(stderr, "\x1B[1;34mUsername %s requested.\n", username);

    while(ptr != NULL) {
        printf("In user verifcation while loop: %s\n", ptr->username);
        if(strcmp(ptr->username, username) == 0) {
            //username taken

            fprintf(stderr, "\x1B[1;31mUsername %s taken.\n", username);
            Close(newUser->connfd);
            Free(newUser);

            //send a message?
            return NULL;
        }
        ptr = ptr->next;
    }

    fprintf(stdout, "\x1B[1;34mUser %s saved to list.\n", username);

    //username not there, save user in list
    if(users_head == NULL) {
        users_head = newUser;
        users_tail = users_head;
    }else{
        users_tail->next = newUser;
        users_tail = users_tail->next;
    }

    users_tail->username = (char*)Malloc(sizeof(char) * strlen(username));
    strcpy(users_tail->username, username);

    fprintf(stderr, "\x1B[1;34mHead's username %s\n", users_head->username);
    //prompt for password

    //Close(connfd), done in comm?
    //clear user
    return NULL;
}

void *commThread(){
    // return if no users
    while(1){
        if(users_head == NULL)
            return NULL;
    }
}

void command(void) {
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0); /* EOF */

    printf("%s", buf); /* Process the input command */
    fflush(stdout);

    if (strcmp(buf, "/help") == 0) {
        // list all server commands
        fprintf(stderr, "/help entered\n");
    }
    else if (strncmp(buf, "/users", 6) == 0) {
        fprintf(stderr, "/users entered\n");
        // dump lists of currently logged users
        struct user *ptr = users_head;
        fprintf(stdout, "|-----User-----|---Address---|--Port--|\n");
        while(ptr != NULL) {
            struct sockaddr_in *sin  = (struct sockaddr_in *)&ptr->sockaddr;

            fprintf(stdout, "|%14s|%14s|%6d\n", ptr->username, inet_ntoa(sin->sin_addr),
            (int) ntohs(sin->sin_port) );
            ptr = ptr->next;
        }
    }
    else if (strcmp(buf, "/shutdown") == 0) {
        // disconnect all users, save states, close sockets, files, free heap
    }
    return;
}