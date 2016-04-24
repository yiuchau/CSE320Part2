#include "csapp.h"
void command(int clientfd);

char buf[MAXLINE];
char *protocol_end = " \r\n\r\n";
int plen = 5;
size_t buf_used;

void read_socket(int fd) {
}

int main(int argc, char **argv) 
{
    int clientfd;
    char *username, *host;
    int port;
    fd_set read_set, ready_set;
    //plen = strlen(protocol_end);

    if (argc < 4) {
    //fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
    //print usage
    exit(0);
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


    fprintf(stderr, "buf:%s\n", buf);

    if(strncmp("EIFLOW \r\n\r\n", buf, 11) == 0) {

        memset(buf, 0, sizeof(buf));
        
        strncpy(buf, "I AM ", 5);
        strncpy(&buf[5], username, strlen(username));
        strncpy(&buf[5 + strlen(username)], protocol_end, plen);
        Write(clientfd, buf, strlen(buf));
        
        /*
        Write(clientfd, "I AM ", 5);
        Write(clientfd, username, strlen(username));
        Write(clientfd, protocol_end, plen);
        */

        memset(buf, 0, sizeof(buf));

        Read(clientfd, buf, sizeof(buf));


        if(strncmp("HI ", buf, 3) == 0 &&
            strncmp(username, &buf[3], strlen(username)) == 0 &&
            strncmp(protocol_end, &buf[3 + strlen(username)], plen) == 0) {
                //successfully connected!
            memset(buf, 0, sizeof(buf));
            Read(clientfd, buf, sizeof(buf));
            Write(STDOUT_FILENO, &buf[5], (size_t)(strstr(buf, protocol_end) - &buf[4]));
        }else{    
            fprintf(stderr, "Expected: HI <Username>, invalid response from server: %s\n", buf);
        }
    }else{
        fprintf(stderr, "Expected: EIFLOW, invalid response from server: %s\n", buf);
    }

    //Write(clientfd, username, strlen(username));
    // check error on login, print usage
     

    FD_ZERO(&read_set);              /* Clear read set */ 
    FD_SET(STDIN_FILENO, &read_set); /* Add stdin to read set */ 
    FD_SET(clientfd, &read_set);     /* Add listenfd to read set */ 

    while (1) {
        ready_set = read_set;
        Select(clientfd+1, &ready_set, NULL, NULL, NULL); 
        if (FD_ISSET(STDIN_FILENO, &ready_set)) {
            //fprintf(stderr, "\x1B[1;34mReading command line from stdin.\n");
            //fflush(stderr);
            command(clientfd); /* Read command line from stdin */
        }
        if (FD_ISSET(clientfd, &ready_set)) { 
            /* Read from clientfd */
            //fprintf(stderr, "\x1B[1;34mReading command line from clientfd.\n");
            //fflush(stderr);
        }
    }



    Close(clientfd);
    exit(0);
}

void command(int clientfd) {
    char buf[MAXLINE];
    if (!Fgets(buf, MAXLINE, stdin))
        exit(0); /* EOF */

    printf("%s", buf); /* Process the input command */

    if (strcmp(buf, "/time") == 0) {
        // asks server for seconds elapsed
    } 
    else if (strcmp(buf, "/help") == 0) {
        // list all client commands
    }
    else if (strcmp(buf, "/listu") == 0) {
        // dump lists of currently logged users
    }
    else if (strcmp(buf, "/logout") == 0) {
        // disconnect with server

        Close(clientfd);
        exit(0);
    }
    return;
}
/* $end select */


