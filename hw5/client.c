#define _GNU_SOURCE
#include "csapp.h"
#include <unistd.h>

void command();

void serverCommand();

char *protocol_end = " \r\n\r\n";
int plen = 5;
size_t buf_used;
char* username;
char **envp;

int clientfd;

int vflag = 0, cflag = 0;



struct chat {
    char * chatname;
    pid_t pid;
    int chatfd;
    struct chat *next;
}*chat_head;


struct chat* Remove(struct chat *ptr){
    struct chat *tptr;

    for(tptr = chat_head; tptr != NULL; tptr = tptr->next) {
        if(tptr->next == ptr){
            tptr->next = ptr->next;
        }
    }

    if(ptr == chat_head){
        chat_head = chat_head->next;
    }

    tptr = ptr->next;

    fprintf(stderr, "Deleting chat for: %s\n", ptr->chatname);
    Free(ptr->chatname);
    Close(ptr->chatfd);
    Free(ptr);

    return tptr;
}




int chatFork(char* fromUser, int fromUserLength);


#define Usage printf("./client [-hcv] NAME SERVER_IP SERVER_PORT\n \
-h         Displays help menu & returns EXIT_SUCCESS\n \
-c         Requests server to create a new use\n \
-v         Vebose pring all incoming and outcoming protcol verbs & content. \
NAME       This is the username to display when chatting. \
SERVER_IP  The ipaddress of server to connect to.\
SERVER_PORT Port number to connect to.\n");

void sigintHandler(int sig){

    fprintf(stderr, "\x1B[1;31m :SIGINT CAUGHT Close(%d)\n", clientfd);
    Close(clientfd);
    exit(0);
}

int main(int argc, char **argv) 
{
    //int clientfd;
    char *host;
    int port;
    char opt;

    char buf[MAXLINE];
    fd_set read_set, ready_set;
    //plen = strlen(protocol_end);


    envp = environ;

    if (argc < 4) {
        Usage;
        exit(0);
    }

    while((opt = getopt(argc, argv, "hcv")) != -1) {
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
            case 'c':
                fprintf(stderr, "User account creation requested.\n");
                cflag++;
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


    username = argv[argc-3];
    host = argv[argc-2];
    port = atoi(argv[argc-1]);


    clientfd = Open_clientfd(host, port);
    // write username to server to login following wolfie protcol
    //strcpy(buf, "WOLFIE \r\n\r\n");
    Write(clientfd, "WOLFIE", 6);
    Write(clientfd, protocol_end, plen);

    memset(buf, 0, sizeof(buf));

    Read(clientfd, buf, sizeof(buf));

    //fprintf(stderr, "buf:%s\n", buf);

    if(strncmp("EIFLOW \r\n\r\n", buf, 11) == 0) {

        memset(buf, 0, sizeof(buf));
        
        strncpy(buf, "I AM ", 5);
        strncpy(&buf[5], username, strlen(username));
        strncpy(&buf[5 + strlen(username)], protocol_end, plen);
        Write(clientfd, buf, strlen(buf));

        memset(buf, 0, sizeof(buf));

        Read(clientfd, buf, sizeof(buf));


        if(strncmp("HI ", buf, 3) == 0 &&
            strncmp(username, &buf[3], strlen(username)) == 0 &&
            strncmp(protocol_end, &buf[3 + strlen(username)], plen) == 0) {
                //successfully connected!
            memset(buf, 0, sizeof(buf));
            Read(clientfd, buf, sizeof(buf));
            Write(STDOUT_FILENO, &buf[5], (size_t)(strstr(buf, protocol_end) - &buf[4]));

        }else if (strncmp("ERR 00 USER NAME TAKEN", buf, 22) == 0 &&
            strncmp(protocol_end, &buf[22], plen) == 0) {
                //user taken
            if(strncmp("BYE", &buf[22 + plen], 3) == 0 &&
                strncmp(protocol_end, &buf[22 + plen + 3], plen) == 0){
                //write byte
                memset(buf, 0, sizeof(buf));
                strncpy(buf, "BYE", 3);
                strncpy(&buf[3], protocol_end, plen);
                Write(clientfd, buf, strlen(buf));
                fprintf(stderr, "Username taken, closing connection. \n");
                Close(clientfd);
                //exit
                exit(EXIT_FAILURE);
            }else{
                fprintf(stderr, "Expected: BYE, invalid response from server: %s\n", buf);
            }
        }else{

            fprintf(stderr, "Expected: HI <Username> || ERR 00 USER NAME TAKEN, invalid response from server: %s\n", buf);
        }
    }else{
        fprintf(stderr, "Expected: EIFLOW, invalid response from server: %s\n", buf);
    }

    //Write(clientfd, username, strlen(username));
    // check error on login, print usage
     

    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(clientfd, &read_set);     /* Add listenfd to read set */ 


        /*init read_set with conn fds*/
    for(struct chat *ptr = chat_head; ptr != NULL; ptr = ptr->next) {
        FD_SET(ptr->chatfd, &read_set);
        //fprintf(stderr, "Adding user %s to comm thread.\n", ptr->username);    
    }

    while (1) {


        FD_ZERO(&read_set);              /* Clear read set */ 
        FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
        FD_SET(clientfd, &read_set);     /* Add listenfd to read set */ 


        /*init read_set with conn fds*/
        for(struct chat *ptr = chat_head; ptr != NULL; ptr = ptr->next) {
            FD_SET(ptr->chatfd, &read_set);
            //fprintf(stderr, "Adding user %s to comm thread.\n", ptr->username);    
        }


        ready_set = read_set;
        Select(FD_SETSIZE, &ready_set, NULL, NULL, NULL);
        for(int i = 0; i < FD_SETSIZE; i++) {
            if(FD_ISSET(i, &ready_set)) {
                if(i == STDIN_FILENO)
                    command();
                else if(i == clientfd) 
                    serverCommand();
                else {
                    //Read from chat, put into protocol and send
                    char input[MAXLINE], output[MAXLINE];
                    memset(input, 0 , MAXLINE);
                    memset(output, 0, MAXLINE);
                    struct chat *ptr;
                    for(ptr = chat_head; ptr != NULL; ptr = ptr->next) {
                        if(ptr->chatfd == i)
                            break;
                    }
                    fprintf(stderr, "\x1B[1;34mReading from chatfd %d.\n", i);

                    if(fcntl(i, F_GETFD) == -1) {
                        fprintf(stderr, "\x1B[1;34m%d chatfd invalid.\n", i);
                        if(ptr != NULL)
                            Remove(ptr);
                        FD_CLR(i, &read_set);
                    }else if(Read(i, input, MAXLINE) <= 0){
                        fprintf(stderr, "\x1B[1;34mChat at chatfd %d disconnected.\n", i);
                        if(ptr != NULL)
                            Remove(ptr);
                        FD_CLR(i, &read_set);
                    }else {
                        fprintf(stderr, "Received msg %s from chat with %s\n", input, ptr->chatname);
                        int offset, toLength, msgLength;
                        memset(output, 0, sizeof(output));
                        strncpy(output, "MSG ", 4);
                        offset = 4;
                        toLength = strlen(ptr->chatname);
                        strncpy(&output[offset], ptr->chatname, toLength);
                        offset += toLength;//1 for space
                        output[offset] = ' ';
                        offset++;
                        strncpy(&output[offset], username, strlen(username));
                        offset += strlen(username);
                        output[offset] = ' ';
                        offset++;
                        msgLength = strlen(input);//subtract space indexss
                        strncpy(&output[offset], input, msgLength);
                        offset += msgLength;
                        strncpy(&output[offset], protocol_end, plen);
                        Write(clientfd, output, sizeof(output));
                        fprintf(stderr, "toLength:%d , msgLength:%d\n", toLength, msgLength);
                    }

                }

            }
        }

        /*
        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
            //fprintf(stderr, "\x1B[1;34mReading command line from stdin.\n");
            //fflush(stderr);
            command(); 
        }else if (FD_ISSET(clientfd, &ready_set)) { 
            fprintf(stderr, "\x1B[1;34mReading command line from clientfd.\n");
            //fflush(stderr);
            serverCommand();
        }
        */
        
    }

    Close(clientfd);
    exit(0);
}

void command() {
    char cmd[MAXLINE], output[MAXLINE];
    if (!Fgets(cmd, MAXLINE, stdin))
        exit(0); /* EOF */

    printf("%s", cmd); /* Process the input command */

    if (strncmp(cmd, "/time", 5) == 0) {

        // asks server for seconds elapsed
        /*
        time_t time_elapsed;
        int hours, minutes, seconds;
        */

        fprintf(stderr, "\x1B[1;34mClient command: /time entered\n");

        memset(output, 0, sizeof(output));
        strncpy(output, "TIME", 4);
        strncpy(&output[4], protocol_end, plen);
        Write(clientfd, output, sizeof(output));

        /*
        memset(input, 0, sizeof(input));
        Read(clientfd, input, sizeof(input));

        if(strncmp("EMIT", input, 4) == 0 &&
            strstr(input, protocol_end) != 0){

            time_elapsed = atoi(&input[5]);
            hours = (int)time_elapsed / 3600;
            time_elapsed = time_elapsed % 3600;
            minutes = (int)time_elapsed / 60;
            seconds = time_elapsed % 60;
            fprintf(stdout, "connected for %d hours %d minutes %d seconds\n", hours, minutes, seconds);

        }else{
            fprintf(stderr, "Expected: TIME, invalid response from server: %s\n", input);
        }
        */

    } 
    else if (strncmp(cmd, "/help", 5) == 0) {
        // list all client commands

        fprintf(stderr, "\x1B[1;34mClient command: /help entered\n");
        fprintf(stdout, "/help: Print all client commands\n\
                         /time: Get time elapsed for connection\n\
                         /listu: Print all currently logged users\n\
                         /chat: message another user\n\
                         /logout: Disconnect with server.\n");
    }
    else if (strncmp(cmd, "/listu", 6) == 0) {
        // dump lists of currently logged users
        
        fprintf(stderr, "\x1B[1;34mClient command: /listu entered\n");
        memset(output, 0, sizeof(output));
        strncpy(output, "LISTU", 5);
        strncpy(&output[5], protocol_end, plen);
        Write(clientfd, output, sizeof(output));

/*
        memset(input, 0, sizeof(input));
        Read(clientfd, input, sizeof(input));

        
        if(strncmp("UTSIL", input, 5) == 0 &&
            strstr(&input[5], protocol_end) != NULL){

            int offset = 6;

            fprintf(stdout, "-----Logged In Users-----\n");
            while(strstr(&input[offset], protocol_end) != NULL) {

                fprintf(stderr, "Offset: %d", offset);
                //parse the users
                Write(STDOUT_FILENO, &input[offset], (int)(strstr(&input[offset], " \r\n") - &input[offset]));
                offset += (int)(strstr(&input[offset], " \r\n") - &input[offset]) + 4;
                Write(STDOUT_FILENO, "\n", 1);
            }

        }else{
            fprintf(stderr, "Expected: UTSIL, invalid response from server: %s\n", input);
        }
        */



    }
    else if (strncmp(cmd, "/logout", 7) == 0) {
        // disconnect with server

        struct chat* ptr = chat_head;
        fprintf(stderr, "\x1B[1;34mClient command: /logout entered\n");

        memset(output, 0, sizeof(output));
        strncpy(output, "BYE", 3);
        strncpy(&output[3], protocol_end, plen);
        Write(clientfd, output, sizeof(output));

        while(ptr != NULL) {
        fprintf(stderr, "Deleting chat: %s\n", ptr->chatname);
        //send bye to all clients
        kill(SIGINT, ptr->pid);
        ptr = Remove(ptr);
    }

        /*
        memset(input, 0, sizeof(input));
        Read(clientfd, input, sizeof(input));

        Close(clientfd);
        exit(0);
        */
    }
    else if(strncmp(cmd, "/chat", 5) == 0) {
        int offset, toLength, msgLength;
        fprintf(stderr, "\x1B[1;34mClient command: /chat entered\n");   
        memset(output, 0, sizeof(output));
        strncpy(output, "MSG ", 4);
        offset = 4;
        toLength = (int)(strstr(&cmd[6], " ") - &cmd[6]);
        strncpy(&output[offset], &cmd[6], toLength);
        offset += toLength;//1 for space
        output[offset] = ' ';
        offset++;
        strncpy(&output[offset], username, strlen(username));
        offset += strlen(username);
        output[offset] = ' ';
        offset++;
        msgLength = strlen(&cmd[6 + toLength + 1]) - 1;//subtract space indexss
        strncpy(&output[offset], &cmd[6 + toLength + 1], msgLength);
        offset += msgLength;
        strncpy(&output[offset], protocol_end, plen);
        Write(clientfd, output, sizeof(output));
        fprintf(stderr, "toLength:%d , msgLength:%d\n", toLength, msgLength);
    }
    return;
}

void serverCommand() {
    char input[MAXLINE];

    printf("Input from socket.\n");

    memset(input, 0, sizeof(input));
    if(Read(clientfd, input, sizeof(input)) <= 0) {
        close(clientfd);
        printf("Server has closed.\n");
    }

    if(strncmp("EMIT", input, 4) == 0 &&
            strstr(input, protocol_end) != 0){

        // asks server for seconds elapsed
        time_t time_elapsed;
        int hours, minutes, seconds;

            time_elapsed = atoi(&input[5]);
            hours = (int)time_elapsed / 3600;
            time_elapsed = time_elapsed % 3600;
            minutes = (int)time_elapsed / 60;
            seconds = time_elapsed % 60;
            fprintf(stdout, "connected for %d hours %d minutes %d seconds\n", hours, minutes, seconds);

        

        /*else{
            fprintf(stderr, "Expected: TIME, invalid response from server: %s\n", input);
    }*/
    }
    else if(strncmp("UTSIL", input, 5) == 0 &&
        strstr(&input[5], protocol_end) != NULL){

        int offset = 6;

        fprintf(stdout, "-----Logged In Users-----\n");
        while(strstr(&input[offset], protocol_end) != NULL) {
            fprintf(stderr, "Offset: %d", offset);
            //parse the users
            Write(STDOUT_FILENO, &input[offset], (int)(strstr(&input[offset], " \r\n") - &input[offset]));
            offset += (int)(strstr(&input[offset], " \r\n") - &input[offset]) + 4;
            Write(STDOUT_FILENO, "\n", 1);
        }

    /*
    else{
        fprintf(stderr, "Expected: UTSIL, invalid response from server: %s\n", input);
    }*/
    }
    else if(strncmp("BYE", input, 3) == 0 &&
        strstr(&input[3], protocol_end) != NULL){
        close(clientfd);
        fprintf(stdout, "Disconnected from server.\n");
        exit(EXIT_SUCCESS);
    }else if(strncmp("ERR", input, 3) == 0 &&
        strstr(&input[3], protocol_end) != NULL){
        fprintf(stdout, "%s\n", input);
    }
    else if(strncmp("MSG", input, 3) == 0 && 
        strstr(&input[3], protocol_end) != NULL) {
        
        int toUserLength, fromUserLength, messageLength;
        char* toUser, *fromUser, *message;
        toUser = &input[4];
        toUserLength = (int)(strstr(toUser, " ") - toUser);
        fromUser = &input[4 + toUserLength + 1];
        fromUserLength = (int)(strstr(fromUser, " ") - fromUser);
        message = fromUser + fromUserLength + 1;
        messageLength = (int)(strstr(message, protocol_end) - message);

        fprintf(stdout, "MSG from server: \n%s\n", input);
        fprintf(stderr, "From Length: %d , To Length: %d\n", fromUserLength, toUserLength);
                   

        //check if window is open for from user, 
        //if doesn't exist
        if(strncmp(username, fromUser, fromUserLength) == 0){
            for(struct chat *ptr = chat_head; ptr != NULL ; ptr = ptr->next) {
                if(strncmp(ptr->chatname, toUser, toUserLength) == 0) {
                    //user chat exists
                    fprintf(stderr, "Chat with user %s exists\n", toUser);
                    Write(ptr->chatfd, message, messageLength);
                    return;
                }
            }
            //user doesn't exist

            fprintf(stderr, "Creating chat with user %s\n", toUser);
            Write(chatFork(toUser, toUserLength), message, messageLength);
        }
        else{
            for(struct chat *ptr = chat_head; ptr != NULL ; ptr = ptr->next) {
                if(strncmp(ptr->chatname, fromUser, fromUserLength) == 0) {
                    //user chat exists

                    fprintf(stderr, "Chat with user %s exists\n", fromUser);
                    Write(ptr->chatfd, message, messageLength);
                    return;
                }
            }
            //user doesn't exist

            fprintf(stderr, "Creating chat with user %s\n", fromUser);
            Write(chatFork(fromUser, fromUserLength), message, messageLength);
        }

    }   
}

int chatFork(char* fromUser, int fromUserLength){
    char *chatname = Malloc(sizeof(char) * fromUserLength + 1);
    strncpy(chatname, fromUser, fromUserLength);
    chatname[fromUserLength] = '\0';
    int fd[2];
    pid_t pid;

    socketpair(AF_UNIX, SOCK_STREAM, 0, fd);

    pid = fork();
    if(pid == 0) {
        char buffer[10];

        char*argv[] = {"/usr/bin/xterm","-geometry","45x35+25","-T", chatname,"-e","./chat", buffer ,NULL};
        snprintf(buffer, 10, "%d", fd[1]);        
        Close(fd[0]);
        execvpe(argv[0], argv, envp);

    }else{
        struct chat *newChat = Malloc(sizeof(struct chat));
        newChat->chatname = chatname;
        newChat->chatfd = fd[0];
        newChat->pid = pid;

        if(chat_head == NULL){
            chat_head = newChat;
            newChat->next = NULL;
        }else{
            newChat->next = chat_head;
            chat_head = newChat;
        }
    }
    
    Close(fd[1]);
    return fd[0];
}


