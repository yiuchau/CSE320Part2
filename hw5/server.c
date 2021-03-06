#include "csapp.h"
#include "time.h"
//#include "cse320.h"

int listenfd;
char *protocol_end = " \r\n\r\n";
int plen = 5;
char *MOTD;
pthread_t commtid;

void echo(int connfd);
void *loginThread(void *vargp);
void *commThread(void* vargp);
void command(void);
void endServer(void);


struct user* Remove(struct user *ptr);


struct user {
    time_t login_time;
    char * username;
    int connfd;
    //ipaddr
    struct sockaddr_storage sockaddr;
    struct user *next;
}*users_head;


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
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    struct user *newUser;
    fd_set read_set, ready_set;

    if (argc < 3) {
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


    MOTD = argv[argc - 1];
    //signal handler to close socket upon ctrl-c
    Signal(SIGINT, sigintHandler);



    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(listenfd, &read_set);     /* Add listenfd to read set */

    while (1) {

        ready_set = read_set;

        Select(FD_SETSIZE, &ready_set, NULL, NULL, NULL); 

        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
            //fprintf(stderr, "\x1B[1;34mReading command line from stdin.\n");
            //fflush(stderr);
            command(); /* Read command line from stdin */
        }

        if (FD_ISSET(listenfd, &ready_set)) { 
            fprintf(stderr, "\x1B[1;34mReading from listenfd: New connection.\n");
            //fflush(stderr);
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
    char* username = (char*) Malloc(sizeof(char) * MAXLINE);
    char buf[MAXLINE];
    int connfd = newUser->connfd;
    //struct user *usersPtr = users_head;
    Pthread_detach(pthread_self()); 
    //Free(vargp); 

    fprintf(stderr, "\x1B[1;34mNew login thread created for %d.\n", newUser->connfd);

    // Communicate with user according to wolfie protocol


    Read(connfd, buf, MAXLINE);

    if(strncmp(buf, "WOLFIE", (size_t)(strstr(buf, " \r\n\r\n") - buf)) == 0) {
        fprintf(stderr, "\x1B[1;34m%s received.\n", buf);

        /*
        strncpy(buf, "I AM ", 5);
        strncpy(&buf[5], username, strlen(username));
        strncpy(&buf[5 + strlen(username)], " \r\n\r\n", 5);
        Write(clientfd, buf, strlen(buf));
        */

        memset(buf, 0, sizeof(buf));
        strncpy(buf, "EIFLOW", 6);
        strncpy(&buf[6], protocol_end, plen + 1);
        Write(connfd, buf, strlen(buf));

        //Write(connfd, "EIFLOW", 6);
        //Write(connfd, protocol_end, plen);

        memset(buf, 0, sizeof(buf));
        
        Read(connfd, buf, sizeof(buf));

        if(strncmp(buf, "I AM", 4) == 0 &&
            strstr(buf, protocol_end) != NULL) {

            fprintf(stderr, "\x1B[1;34m%s received.\n", buf);
            memset(username, 0, sizeof(&username));
            fprintf(stderr, "Test length: %d\n", (int)(strstr(buf, protocol_end) - &buf[5]));
            strncpy(username, &buf[5], (size_t)(strstr(buf, protocol_end) - &buf[5]));

        }else{

            fprintf(stderr, "Expected: I AM <Username>, invalid response from client: %s\n", buf);
        }
    }else{

        fprintf(stderr, "Expected: WOLFIE, invalid response from client: %s\n", buf);
    }


    fprintf(stderr, "\x1B[1;34mUsername %s requested.\n", username);

    while(ptr != NULL) {
        fprintf(stderr,"In user verification while loop: %s\n", ptr->username);
        if(strcmp(ptr->username, username) == 0) {
            //username taken

            fprintf(stderr, "\x1B[1;31mUsername %s taken.\n", username);
            //comm with client


            Free(newUser);
            Free(username);

            memset(buf, 0, sizeof(buf));
            strncpy(buf, "ERR 00 USER NAME TAKEN", 22);
            strncpy(&buf[22], protocol_end, plen);
            strncpy(&buf[22 + plen], "BYE", 3);
            strncpy(&buf[22 + plen + 3], protocol_end, plen);
            Write(connfd, buf, strlen(buf));

            memset(buf, 0, sizeof(buf));
        
            Read(connfd, buf, sizeof(buf));

            if(strncmp(buf, "BYE", 3) == 0 &&
                strncmp(&buf[3], protocol_end, plen) == 0) {

                fprintf(stderr, "\x1B[1;34m%s received.\n", buf);
            }else{
                fprintf(stderr, "Expected: BYE, invalid response from client: %s\n", buf);
            }

            Close(connfd);

            //send a message?
            return NULL;
        }
        ptr = ptr->next;
    }


    memset(buf, 0, sizeof(buf));
    strncpy(buf, "HI ", 3);
    strncpy(&buf[3], username, strlen(username));
    strncpy(&buf[3 + strlen(username)], protocol_end, plen);
    Write(connfd, buf, strlen(buf));


    newUser->login_time = time(NULL);
    newUser->username = username;
    //username not there, save user in list
    if(users_head == NULL) {
        //spawn comm thread

        Pthread_create(&commtid, NULL, commThread, NULL);
        newUser->next = NULL;
    }else{
        newUser->next = users_head;
    }


    users_head = newUser;
    //strcpy(users_tail->username, username);



    fprintf(stderr, "\x1B[1;34mUser %s saved to list.\n", username);
    //send MOTD


    memset(buf, 0, sizeof(buf));
    strncpy(buf, "MOTD ", 5);
    strncpy(&buf[5], MOTD, strlen(MOTD));
    strncpy(&buf[5 + strlen(MOTD)], protocol_end, plen);
    Write(connfd, buf, strlen(buf));

    fprintf(stderr, "\x1B[1;34mHead's username %s\n", users_head->username);
    //prompt for password

    //Close(connfd), done in comm?
    //clear user
    return NULL;
}

void *commThread(void* vargp){
    // return if no users

    fd_set read_set, ready_set;
    struct user *ptr;
    //char test[1];



    fprintf(stderr, "\x1B[1;34mNew communication thread created.\n");



    FD_ZERO(&read_set); /* Clear read set */

    /*init read_set with conn fds*/
    for(ptr = users_head; ptr != NULL; ptr = ptr->next) {
        FD_SET(ptr->connfd, &read_set);
        fprintf(stderr, "Adding user %s to comm thread.\n", ptr->username);    
    }


    while(1){
        if(users_head == NULL)
            return NULL;

        FD_ZERO(&read_set); /* Clear read set */

        /*init read_set with conn fds*/
        for(ptr = users_head; ptr != NULL; ptr = ptr->next) {
            FD_SET(ptr->connfd, &read_set);
            fprintf(stderr, "Adding user %s to comm thread.\n", ptr->username);    
        }


        ready_set = read_set;

        Select(FD_SETSIZE, &ready_set, NULL, NULL, NULL);
        for(int i = 0; i < FD_SETSIZE; i++) {
            if(FD_ISSET(i, &ready_set)) {
                //perform read

                char input[MAXLINE], output[MAXLINE];
                memset(input, 0, MAXLINE);
                memset(output, 0, MAXLINE);

                fprintf(stderr, "\x1B[1;34mReading from connfd %d.\n", i);

                for(ptr = users_head; ptr != NULL; ptr = ptr->next) {
                    if(ptr->connfd == i)
                        break;
                }

                if(fcntl(i, F_GETFD) == -1) {
                    fprintf(stderr, "\x1B[1;34m%d Connfd invalid.\n", i);
                    if(ptr != NULL)
                        Remove(ptr);
                    FD_CLR(i, &read_set);
                }else if(Read(i, input, MAXLINE) <= 0){
                    fprintf(stderr, "\x1B[1;34mClient at connfd %d disconnected.\n", i);
                    if(ptr != NULL)
                        Remove(ptr);
                    FD_CLR(i, &read_set);
                }else if (strncmp(input, "TIME", 4) == 0 &&
                        strncmp(&input[4], protocol_end, plen) == 0) {

                    fprintf(stderr, "\x1B[1;34mTime called by %s.\n", ptr->username);
                    time_t time_elapsed = time(NULL);
                    time_elapsed -= ptr->login_time;

                    strncpy(output, "EMIT ", 5);
                    sprintf(&output[5], "%d", (int)time_elapsed);
                    strncpy(&output[strlen(output)], protocol_end, plen);
                    Write(i, output, MAXLINE);

                }else if (strncmp(input, "LISTU", 5) == 0 &&
                        strncmp(&input[5], protocol_end, plen) == 0) {

                    int offset = 0;
                    fprintf(stderr, "\x1B[1;34mLISTU called by %s.\n", ptr->username);

                    strncpy(output, "UTSIL ",6);
                    offset = 6;
                    for(struct user *ptr = users_head; ptr != NULL; ptr = ptr->next) {
                        fprintf(stderr, "Offset: %d\n", offset);
                        strncpy(&output[offset], ptr->username, strlen(ptr->username));
                        offset += strlen(ptr->username);
                        if(ptr->next != NULL){
                            strncpy(&output[offset], " \r\n ", 4);
                            offset += 4;
                        }
                    }


                    strncpy(&output[offset], protocol_end, plen);
                    offset += plen;

                    fprintf(stderr, "Offset: %d\n", offset);

                    Write(STDOUT_FILENO, output, MAXLINE);
                    //Add handling for output above MAXLINE chars
                    Write(i, output, MAXLINE);

                }else if(strncmp(input, "BYE", 3) == 0 &&
                    strncmp(&input[3], protocol_end, plen) == 0) {
                    strncpy(output, "BYE", 3);
                    strncpy(&output[3], protocol_end, plen);
                    Write(i, output, sizeof(output));
                    Remove(ptr);
                }else if (strncmp(input, "MSG", 3) == 0 &&
                        strstr(&input[3], protocol_end) != NULL) {
                    int tofd = -1, fromfd = -1, toUserLength, fromUserLength;
                    char* toUser, *fromUser;
                    toUser = &input[4];
                    toUserLength = (int)(strstr(toUser, " ") - toUser);
                    fromUser = &input[4 + toUserLength + 1];
                    fromUserLength = (int)(strstr(fromUser, " ") - fromUser);
                    for(struct user * ptr = users_head; ptr != NULL; ptr = ptr->next){
                        if(strncmp(ptr->username, fromUser, fromUserLength) == 0)
                            fromfd = ptr->connfd;
                        else if(strncmp(ptr->username, toUser, toUserLength) == 0)
                            tofd = ptr->connfd;
                    }
                    if(tofd == -1 || fromfd == -1){
                        char buf[] = "ERR 01 USER NOT AVAILABLE \r\n\r\n";
                        fprintf(stderr, "Invalid user for msg.\n");
                        Write(i, buf, sizeof(buf));
                        return NULL;
                    }else{
                        Write(tofd, input, sizeof(input));
                        Write(fromfd, input, sizeof(input));
                    }

                }
            }
        } 

    }

    return NULL;
}

struct user* Remove(struct user *ptr){
    struct user *tptr;

    for(tptr = users_head; tptr != NULL; tptr = tptr->next) {
        if(tptr->next == ptr){
            tptr->next = ptr->next;
        }
    }

    if(ptr == users_head){
        users_head = users_head->next;
    }

    tptr = ptr->next;

    fprintf(stderr, "Deleting user: %s\n", ptr->username);
    Free(ptr-> username);
    Close(ptr->connfd);
    Free(ptr);

    return tptr;
}

void command(void) {
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0); /* EOF */

    printf("\x1B[0m%s", buf); /* Process the input command */
    //fflush(stdout);

    if (strncmp(buf, "/help", 5) == 0) {
        // list all server commands
        fprintf(stderr, "\x1B[1;34mSystem command: /help entered\n");
        fprintf(stdout, "/help: Print all system commands\n\
                         /users: Print all currently logged users\n\
                         /shutdown: Cleanly disconnect all users.\n");
    }
    else if (strncmp(buf, "/users", 6) == 0) {
        fprintf(stderr, "\x1B[1;34mSystem command: /users entered\n");
        // dump lists of currently logged users
        struct user *ptr = users_head;
        fprintf(stdout, "|-----User-----|---Address---|--Port--|\n");
        while(ptr != NULL) {
            struct sockaddr_in *sin  = (struct sockaddr_in *)&ptr->sockaddr;

            fprintf(stdout, "|%14s|%13s|%8d|\n", ptr->username, inet_ntoa(sin->sin_addr),
            (int) ntohs(sin->sin_port) );
            ptr = ptr->next;
        }
    }
    else if (strncmp(buf, "/shutdown", 9) == 0) {
        // disconnect all users, save states, close sockets, files, free heap

        fprintf(stderr, "\x1B[1;34mSystem command: /shutdown entered\n");
        endServer();
    }
    return;
}

void endServer() {
    struct user *ptr = users_head;
    char output[MAXLINE];

    while(ptr != NULL) {
        fprintf(stderr, "Deleting user: %s\n", ptr->username);
        //send bye to all clients
        strncpy(output, "BYE", 3);
        strncpy(&output[3], protocol_end, plen);
        Write(ptr->connfd, output, sizeof(output));
        ptr = Remove(ptr);
    }

    users_head = NULL;

    return;
}